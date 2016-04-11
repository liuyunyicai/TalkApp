#ifndef NETWORK_H
#define NETWORK_H


/*	packet head definition	*/

#define ALIGN_HEAD					(0x55555)
#define AUDIO_TYPE					('A')
#define VIDEO_TYPE					('V')
#define OPEN_LOCK					('O')
#define OPEN_LOCK_ACKNOWLEDGE_FALG	('K')

typedef enum frame_type
{
	AFrame = 0,		/* Audio Frame		*/
	IFrame = 0,		/* Video I Frame	*/
	PFrame = 1		/* Video P Frame	*/
} frame_type_t;

typedef enum
{
	open_lock_msg = 0,
	open_lock_ack_msg
} special_msg_t;

typedef struct stream_head
{
	unsigned long  align;			/*	ALIGN_HEAD						*/
	unsigned char  msg_type;		/*	AUDIO_TYPE  VIDEO_TYPE			*/
	unsigned char  frame_type;		/*	AFrame  IFrame  PFrame			*/
	unsigned char  rsv[3];			/*	Rsv[0] denotes frame ID, 0~254	*/
	unsigned long  size;			/*	codec head size plus data size	*/
} stream_head_t, *pstream_head_t;


/*	frame head definition		*/

#define SYN_START_CODE				(0xA0A46801)
#define SYN_END_CODE				(0xF0F46802)

#define VIDEO_FRAME					(0)
#define AUDIO_FRAME					(1)
#define I_FRAME						(0)
#define P_FRAME						(1)

#define MPG4_ENCODE					(0)

#define ADPCM_ENCODE				(0)
#define G729_ENCODE					(1)
#define G721_ENCODE					(2)

typedef struct codec_head
{
	unsigned long   syncode;		/*	SYN_START_CODE							*/
	unsigned short  width;
	unsigned short  height;
	unsigned char   stream_type;	/*	VIDEO_FRAME  AUDIO_FRAME				*/
	unsigned char   frame_type;		/*	I_FRAME P_FRAME	for video, 0 for audio	*/
	unsigned short  codec_info;		/*	encode type								*/
	unsigned long   time_stamp;
	unsigned long   serial_num;		/*	frame ID, independent for audio & video	*/
	unsigned long   frame_size;		/*	data size, plus verification size		*/
	
} codec_head_t, *pcodec_head_t;


int wait_for_client();

int start_client();

void stop_network();

int send_audio_frame(const void *data, int size);

int read_audio_frame(void *data, int max_size);

int send_video_frame(const void *data, int size);

int read_video_frame(void *data, int max_size);

void set_server_ip(const char *ip);

#endif
