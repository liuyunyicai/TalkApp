#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include <android/log.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "audio.h"
#include "adpcm.h"
#include "network.h"
#include "circular_queue.h"

#define LOG_TAG		"BT_NATIVE_AUDIO"
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()	LOGD("current line: %d, function: %s", __LINE__, __func__)


#define SAMPLE_RATE			SL_SAMPLINGRATE_8				//	8000
#define BITS_PER_SAMPLE		SL_PCMSAMPLEFORMAT_FIXED_16		//	16
#define DURATION_PER_FRAME	40								//	40 millisecond each frame

#define SAMPLE_PER_FRAME	320								//	8000 / 1000 * 40
#define BYTES_PER_FRAME		640								//	SAMPLE_PER_FRAME * 2
#define WORDS_PER_FRAME		320								//	BYTES_PER_FRAME / 2

#define INVALID_PID 		((pthread_t) -1)


static SLObjectItf	engine_obj = NULL;
static SLEngineItf	engine_itf = NULL;

static SLObjectItf	recorder_obj = NULL;
static SLRecordItf	recorder_itf = NULL;
static SLAndroidSimpleBufferQueueItf recorder_bq_itf = NULL;

static SLObjectItf	player_obj = NULL;
static SLPlayItf	player_itf = NULL;
static SLObjectItf	output_obj = NULL;
static SLAndroidSimpleBufferQueueItf player_bq_itf = NULL;


static circular_queue *recorder_cq;

static void *adpcm_encoder = NULL;
static void *adpcm_decoder = NULL;

static sem_t player_sem;

static short	recorder_bq_buf	[WORDS_PER_FRAME];
static short	player_bq_buf	[WORDS_PER_FRAME];

static short	recorder_buf	[WORDS_PER_FRAME];

static uint8_t	encode_buf		[WORDS_PER_FRAME];
static uint8_t	decode_buf		[WORDS_PER_FRAME];

static pthread_t recorder_thread	= INVALID_PID;
static pthread_t player_thread		= INVALID_PID;


static int audio_on = 0;
static int rec_num = 0;
static int play_num = 0;


int create_resources()
{
	int ret;

	recorder_cq = cq_create(32);
	if (recorder_cq == NULL)
	{
		LOGD("create recorder queue failed");
		return -1;
	}

	sem_init(&player_sem, 0, 1);

	ret = ADPCM_enc_init(&adpcm_encoder, AUDIO_PCM_16_BIT);
	if (ret != 0)
	{
		LOGD("create ADPCM encoder failed");
		return -1;
	}

	ret = ADPCM_dec_init(&adpcm_decoder, AUDIO_PCM_16_BIT);
	if (ret != 0)
	{
		LOGD("create ADPCM decoder failed");
		return -1;
	}

	return 0;
}

void destroy_resources()
{
	if (adpcm_decoder != NULL)
	{
		ADPCM_dec_free(adpcm_decoder);
		adpcm_decoder = NULL;
	}

	if (adpcm_encoder != NULL)
	{
		ADPCM_enc_free(adpcm_encoder);
		adpcm_encoder = NULL;
	}

	sem_destroy(&player_sem);

	if (recorder_cq != NULL)
	{
		cq_destroy(recorder_cq);
		recorder_cq = NULL;
	}
}

int create_audio_engine()
{
	SLresult result;

	result = slCreateEngine(&engine_obj, 0, NULL, 0, NULL, NULL);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("create engine failed, %u", result);
		return -1;
	}

	result = (*engine_obj)->Realize(engine_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("realize engine failed, %u", result);
		return -1;
	}

	result = (*engine_obj)->GetInterface(engine_obj, SL_IID_ENGINE, &engine_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("get engine interface failed, %u", result);
		return -1;
	}

	return 0;
}

void destroy_audio_engine()
{
	if (engine_obj != NULL)
	{
		(*engine_obj)->Destroy(engine_obj);

		engine_obj = NULL;
		engine_itf = NULL;
	}
}

void bq_recoder_callback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	rec_num++;
	if (audio_on)
	{
		int bytes = cq_write(recorder_cq, recorder_bq_buf, BYTES_PER_FRAME);
		(*recorder_bq_itf)->Enqueue(recorder_bq_itf, recorder_bq_buf, BYTES_PER_FRAME);
	}
}

int start_recording()
{
	SLresult result;

	result = (*recorder_bq_itf)->Enqueue(recorder_bq_itf, recorder_bq_buf, BYTES_PER_FRAME);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("recorder enqueue failed, %u", result);
		return -1;
	}

	result = (*recorder_itf)->SetRecordState(recorder_itf, SL_RECORDSTATE_RECORDING);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("start audio recording failed, %u", result);
		return -1;
	}

	return 0;
}

int stop_recording()
{
	SLresult result;

	result = (*recorder_itf)->SetRecordState(recorder_itf, SL_RECORDSTATE_STOPPED);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("stop audio recording failed, %u", result);
		return -1;
	}

	result = (*recorder_bq_itf)->Clear(recorder_bq_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("clear audio recorder buffer queue failed, %u", result);
		return -1;
	}

	return 0;
}

int create_audio_recorder()
{
	SLresult result;

	SLDataLocator_IODevice locator_mic =
	{
		SL_DATALOCATOR_IODEVICE,
		SL_IODEVICE_AUDIOINPUT,
		SL_DEFAULTDEVICEID_AUDIOINPUT,
		NULL
	};
	SLDataSource audio_source = { &locator_mic, NULL };

	SLDataLocator_AndroidSimpleBufferQueue locator_bq =
	{
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1
	};
	SLDataFormat_PCM format_pcm =
	{
		SL_DATAFORMAT_PCM,
		1,
		SAMPLE_RATE,
		BITS_PER_SAMPLE,
		BITS_PER_SAMPLE,
		SL_SPEAKER_FRONT_CENTER,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSink audio_sink = { &locator_bq, &format_pcm };


	const SLInterfaceID ids[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	const SLboolean req[1] = { SL_BOOLEAN_TRUE };
	result = (*engine_itf)->CreateAudioRecorder(engine_itf,
	         &recorder_obj, &audio_source, &audio_sink, 1, ids, req);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("create audio recorder failed, %u", result);
		return -1;
	}

	result = (*recorder_obj)->Realize(recorder_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("realize audio recorder failed, %u", result);
		return -1;
	}

	result = (*recorder_obj)->GetInterface(recorder_obj, SL_IID_RECORD, &recorder_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("get audio recorder interface failed, %u", result);
		return -1;
	}

	result = (*recorder_obj)->GetInterface(recorder_obj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorder_bq_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("get recorder buffer queue interface failed, %u", result);
		return -1;
	}

	result = (*recorder_bq_itf)->RegisterCallback(recorder_bq_itf, bq_recoder_callback, NULL);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("register recorder buffer queue callback failed, %u", result);
		return -1;
	}

	if (stop_recording() != 0)
		return -1;

	return 0;
}

void destroy_audio_recorder()
{
	if (recorder_obj != NULL)
	{
		(*recorder_obj)->Destroy(recorder_obj);

		recorder_obj = NULL;
		recorder_itf = NULL;
		recorder_bq_itf = NULL;
	}
}

void bq_player_callback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	play_num++;
	sem_post(&player_sem);
}

int start_playing()
{
	SLresult result;

	result = (*player_itf)->SetPlayState(player_itf, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("start audio playing failed, %u", result);
		return -1;
	}

	return 0;
}

int stop_playing()
{
	SLresult result;

	result = (*player_itf)->SetPlayState(player_itf, SL_PLAYSTATE_STOPPED);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("stop audio playing failed, %u", result);
		return -1;
	}

	result = (*player_bq_itf)->Clear(player_bq_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("clear audio player buffer queue failed, %u", result);
		return -1;
	}

	return 0;
}

int create_audio_player()
{
	SLresult result;

	SLDataLocator_AndroidSimpleBufferQueue locator_bq =
	{
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1
	};
	SLDataFormat_PCM format_pcm =
	{
		SL_DATAFORMAT_PCM,
		1,
		SAMPLE_RATE,
		BITS_PER_SAMPLE,
		BITS_PER_SAMPLE,
		SL_SPEAKER_FRONT_CENTER,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audio_source = { &locator_bq, &format_pcm };


	const SLInterfaceID output_mix_ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean output_mix_req[1] = { SL_BOOLEAN_FALSE };
	result = (*engine_itf)->CreateOutputMix(engine_itf, &output_obj, 1, output_mix_ids, output_mix_req);

	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("create output mix failed, %u", result);
		return -1;
	}

	result = (*output_obj)->Realize(output_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("realize output mix failed, %u", result);
		return -1;
	}

	SLDataLocator_OutputMix locatorOutMix = { SL_DATALOCATOR_OUTPUTMIX, output_obj };
	SLDataSink audio_sink = { &locatorOutMix, NULL };

	const SLInterfaceID player_ids[2] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
	const SLboolean player_req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
	result = (*engine_itf)->CreateAudioPlayer(engine_itf,
	         &player_obj, &audio_source, &audio_sink, 2, player_ids, player_req);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("create audio player failed, %u", result);
		return -1;
	}

	result = (*player_obj)->Realize(player_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("realize audio player failed, %u", result);
		return -1;
	}

	result = (*player_obj)->GetInterface(player_obj, SL_IID_PLAY, &player_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("get audio player interface failed, %u", result);
		return -1;
	}

	result = (*player_obj)->GetInterface(player_obj, SL_IID_BUFFERQUEUE, &player_bq_itf);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("get player buffer queue interface failed, %u", result);
		return -1;
	}

	result = (*player_bq_itf)->RegisterCallback(player_bq_itf, bq_player_callback, NULL);
	if (result != SL_RESULT_SUCCESS)
	{
		LOGD("register player buffer queue callback failed, %u", result);
		return -1;
	}

	if (stop_playing())
		return -1;

	return 0;
}

void destroy_audio_player()
{
	if (player_obj != NULL)
	{
		(*player_obj)->Destroy(player_obj);

		player_obj = NULL;
		player_itf = NULL;
		player_bq_itf = NULL;
	}

	if (output_obj != NULL)
	{
		(*output_obj)->Destroy(output_obj);

		output_obj = NULL;
	}
}


void *recorder_thread_entry(void *arg)
{
	int bytes;
	rec_num = 0;
	clock_t begin = clock();

	while (audio_on)
	{
		bytes = cq_read(recorder_cq, recorder_buf, BYTES_PER_FRAME);

		if (bytes <= 0)
			continue;

		bytes = ADPCM_enc_frame(adpcm_encoder, recorder_buf, encode_buf, SAMPLE_PER_FRAME);

		send_audio_frame(encode_buf, bytes);
	}

	LOGD("%lu frames recorded, used %.0f ms", rec_num, (clock() - begin) * 1000.0 / CLOCKS_PER_SEC);

	return NULL;
}


void *player_thread_entry(void *arg)
{
	int bytes;
	struct timespec ts;

	clock_t begin = clock();
	play_num = 0;

	while (audio_on)
	{
		bytes = read_audio_frame(decode_buf, BYTES_PER_FRAME);
		if (bytes <= 0)
			continue;

		while (audio_on)
		{
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += 2;

			if (sem_timedwait(&player_sem, &ts) == 0)
			{
				bytes = ADPCM_dec_frame(adpcm_decoder, decode_buf, player_bq_buf, bytes);
				(*player_bq_itf)->Enqueue(player_bq_itf, player_bq_buf, BYTES_PER_FRAME);
				break;
			}
		}
	}

	LOGD("%d frames played, used %.0f ms", play_num, (clock() - begin) * 1000.0 / CLOCKS_PER_SEC);

	return NULL;
}


/*****	External API Calls	******/

int start_audio()
{
	int result;

	if (create_resources() != 0)
		return -1;

	if (create_audio_engine() != 0)
		return -1;

	if (create_audio_recorder() != 0)
		return -1;

	if (create_audio_player() != 0)
		return -1;

	audio_on = 1;

	start_recording();

	result = pthread_create(&recorder_thread, NULL, recorder_thread_entry, NULL);
	if (result != 0)
	{
		LOGD("create record thread failed, %d", result);
		return -1;
	}

	start_playing();

	result = pthread_create(&player_thread, NULL, player_thread_entry, NULL);
	if (result != 0)
	{
		LOGD("create play thread failed, %d", result);
		return -1;
	}

	return 0;
}

void stop_audio()
{
	audio_on = 0;

	if (recorder_thread != INVALID_PID)
	{
		pthread_join(recorder_thread, NULL);
		recorder_thread = INVALID_PID;
	}

	if (player_thread != INVALID_PID)
	{
		pthread_join(player_thread, NULL);
		player_thread = INVALID_PID;
	}

	destroy_audio_player();
	destroy_audio_recorder();
	destroy_audio_engine();
	destroy_resources();
}