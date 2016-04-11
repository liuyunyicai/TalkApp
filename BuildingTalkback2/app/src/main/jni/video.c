#include <android/log.h>

#include <time.h>
#include <string.h>

#include <fcntl.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "video.h"
#include "network.h"
#include "native_interface.h"

#include "libffmpeg/include/libavutil/avutil.h"
#include "libffmpeg/include/libavutil/imgutils.h"
#include "libffmpeg/include/libavcodec/avcodec.h"
#include "libffmpeg/include/libswscale/swscale.h"


#define LOG_TAG     	"BT_NATIVE_VIDEO"
#define LOGD(...)   	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()  	LOGD("current line: %d, function: %s", __LINE__, __func__)


#define CHECK_EQUAL(res,expect,ret,...)				\
	if (res == expect)								\
	{												\
		LOGD(__VA_ARGS__);							\
		return ret;									\
	}

#define CHECK_LESS(res,expect,ret,...)				\
	if (res < expect)								\
	{												\
		LOGD(__VA_ARGS__);							\
		return ret;									\
	}


#define CLEAR(arg)  		memset(&(arg), 0, sizeof (arg))

#define DEV_PATH			"/dev/video0"

#define CAPTURE_WIDTH		704
#define CAPTURE_HEIGHT		576
#define CAPTURE_YUYV_SIZE	(CAPTURE_WIDTH * CAPTURE_HEIGHT * 2)


#define WIDTH				352
#define HEIGHT				288
#define RGB_SIZE			(WIDTH * HEIGHT * 4)

#define V4L2_CAPTURE_FMT	V4L2_PIX_FMT_YUYV
#define AV_CAPTURE_FMT		AV_PIX_FMT_YUYV422

#define DISPLAY_FMT			AV_PIX_FMT_RGB32

#define INVALID_PID 		((pthread_t) -1)

#define CAPTURE_BUFFER_NUMBER	5

static int fd;

struct buffer
{
	void *start;
	int length;
};
static struct buffer buffers[CAPTURE_BUFFER_NUMBER];

static uint8_t yuv_buffer[CAPTURE_YUYV_SIZE];
static uint8_t dec_buffer[CAPTURE_YUYV_SIZE];
static uint8_t rgb_buffer[RGB_SIZE];

static int enc_num;
static int dec_num;

static AVCodec *enc_codec = NULL;
static AVCodec *dec_codec = NULL;
static AVCodecContext *enc_ctx = NULL;
static AVCodecContext *dec_ctx = NULL;

static AVFrame *yuyv_frame = NULL;
static AVFrame *enc_frame = NULL;
static AVFrame *dec_frame = NULL;
static AVFrame *rgb_frame = NULL;

static AVPacket enc_pkt;
static AVPacket dec_pkt;

static struct SwsContext *enc_sws = NULL;
static struct SwsContext *dec_sws = NULL;

static int video_on;

static pthread_t capture_thread = INVALID_PID;
static pthread_t display_thread = INVALID_PID;


/*	V4L2 related functions	*/

int init_device()
{
	int ret;

	struct v4l2_capability cap;
	CLEAR(cap);

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);

	if (ret != 0)
	{
		LOGD("VIDIOC_QUERYCAP failed");
		return -1;
	}

	LOGD("driver = %s, card = %s, bus = %s, version = %d.%d, Capabilities = %08x",
	     cap.driver,
	     cap.card,
	     cap.bus_info,
	     (cap.version >> 16) && 0xff,
	     (cap.version >> 24) && 0xff,
	     cap.capabilities);

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		LOGD("V4L2_CAP_VIDEO_CAPTURE is not supported");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		LOGD("V4L2_CAP_STREAMING is not supported");
		return -1;
	}

	struct v4l2_input input;
	CLEAR(input);
	input.index = 0;

	ret = ioctl(fd, VIDIOC_ENUMINPUT, &input);
	if (ret != 0)
	{
		LOGD("VIDIOC_ENUMINPUT failed");
		return -1;
	}

	ret = ioctl(fd, VIDIOC_S_INPUT, &input);
	if (ret != 0)
	{
		LOGD("VIDIOC_S_INPUT failed");
		return -1;
	}

	struct v4l2_fmtdesc fmtdes;
	CLEAR(fmtdes);
	fmtdes.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdes)) == 0)
	{
		if (fmtdes.pixelformat == 0 || fmtdes.index >= 32)
			break;

		LOGD("	VIDIOC_ENUM_FMT index = %d", fmtdes.index);
		LOGD("	pixelformat = %c%c%c%c, description = %s",
		     (fmtdes.pixelformat >>  0) & 0xFF,
		     (fmtdes.pixelformat >>  8) & 0xFF,
		     (fmtdes.pixelformat >> 16) & 0xFF,
		     (fmtdes.pixelformat >> 24) & 0xFF,
		     fmtdes.description);

		fmtdes.index++;
	}

	struct v4l2_format fmt;
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = CAPTURE_WIDTH;
	fmt.fmt.pix.height = CAPTURE_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_CAPTURE_FMT;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret != 0)
	{
		LOGD("VIDIOC_S_FMT failed");
		return -1;
	}

	LOGD("Selected Camera Mode:   Width: %d, Height: %d, PixFmt: %s, Field: %d, sizeimage = %d",
	     fmt.fmt.pix.width,
	     fmt.fmt.pix.height,
	     (char *) &fmt.fmt.pix.pixelformat,
	     fmt.fmt.pix.field,
	     fmt.fmt.pix.sizeimage);

	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = CAPTURE_BUFFER_NUMBER;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &req);

	if (ret != 0 || req.count != CAPTURE_BUFFER_NUMBER)
	{
		LOGD("VIDIOC_REQBUFS failed, ret = %d", ret);
		return -1;
	}
	LOGD("count = %d, type = %d, memory = %d", req.count, req.type, req.memory);

	int n_buffers;
	for (n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret != 0)
		{
			LOGD("VIDIOC_QUERYBUF failed");
			return -1;
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(0, buf.length, PROT_READ | PROT_WRITE,
		                                MAP_SHARED, fd, buf.m.offset);

		if (buffers[n_buffers].start == NULL)
		{
			LOGD("buffers[%d].start == NULL", n_buffers);
			return -1;
		}

		LOGD("Length: %d, offset = %d, Address: %p",
		     buffers[n_buffers].length,
		     buf.m.offset,
		     buffers[n_buffers].start);
	}

	for (n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret != 0)
		{
			LOGD("VIDIOC_QBUF failed");
			return -1;
		}
	}

	return 0;
}

int uninit_device()
{
	int ret;
	int n_buffers;

	// for (n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	// {
	// 	struct v4l2_buffer buf;
	// 	CLEAR(buf);
	// 	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// 	buf.memory = V4L2_MEMORY_MMAP;

	// 	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	// 	if (ret != 0)
	// 	{
	// 		LOGD("VIDIOC_QBUF failed");
	// 		return -1;
	// 	}
	// }

	for (n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{
		ret = munmap(buffers[n_buffers].start, buffers[n_buffers].length);
		if (ret != 0)
		{
			LOGD("munmap failed");
			return -1;
		}
	}
	
	LOGD("device uninit done");

	return 0;
}

struct buffer* v4l2_dqbuf()
{
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	CHECK_LESS(ret, 0, NULL, "VIDIOC_DQBUF failed")

	return &buffers[buf.index];
}

int v4l2_qbuf()
{
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	int ret = ioctl(fd, VIDIOC_QBUF, &buf);
	CHECK_LESS(ret, 0, -1, "VIDIOC_QBUF failed")

	return 0;
}

int v4l2_on()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int ret = ioctl(fd, VIDIOC_STREAMON, &type);
	CHECK_LESS(ret, 0, -1, "VIDIOC_STREAMON failed, ret = %d", ret);

	return 0;
}

int v4l2_off()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	CHECK_LESS(ret, 0, -1, "VIDIOC_STREAMOFF failed")

	return 0;
}


/*	ffmpeg related functions	*/

int coder_init(enum AVCodecID id)
{
	int ret;

	avcodec_register_all();
	LOGD("1");

	enc_codec = avcodec_find_encoder(id);
	CHECK_EQUAL(enc_codec, NULL, -1, "encoding codec not found")
	LOGD("1");

	enc_ctx = avcodec_alloc_context3(enc_codec);
	CHECK_EQUAL(enc_ctx, NULL, -1, "cannot allocate encoding context")
	LOGD("1");

	enc_ctx->bit_rate = 800000;
	enc_ctx->width = WIDTH;
	enc_ctx->height = HEIGHT;
	enc_ctx->time_base = (AVRational) { 1, 15 };
	enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	enc_ctx->max_b_frames = 0;
	enc_ctx->gop_size = 10;

	ret = avcodec_open2(enc_ctx, enc_codec, NULL);
	CHECK_LESS(ret, 0, -1, "cannot open encoding codec")
	LOGD("1");

	yuyv_frame = av_frame_alloc();
	CHECK_EQUAL(yuyv_frame, NULL, -1, "cannot alloc encoding frame")
	ret = avpicture_fill((AVPicture *) yuyv_frame,
	                     yuv_buffer, AV_CAPTURE_FMT, CAPTURE_WIDTH, CAPTURE_HEIGHT);
	CHECK_LESS(ret, 0, -1, "cannot map yuv_buffer to encoding frame")
	LOGD("1");

	enc_frame = av_frame_alloc();
	CHECK_EQUAL(enc_frame, NULL, -1, "cannot alloc encoding frame")
	ret = av_image_alloc(enc_frame->data, enc_frame->linesize,
	                     WIDTH, HEIGHT, AV_PIX_FMT_YUV420P, 32);
	CHECK_LESS(ret, 0, -1, "cannot alloc encoding frame data")
	LOGD("1");

	// enc_sws = sws_getCachedContext(enc_sws,
	//                                CAPTURE_WIDTH, CAPTURE_HEIGHT, AV_CAPTURE_FMT,
	//                                WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
	//                                SWS_POINT, NULL, NULL, NULL);
	enc_sws = sws_getContext(CAPTURE_WIDTH, CAPTURE_HEIGHT, AV_CAPTURE_FMT,
	                         WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
	                         SWS_POINT, NULL, NULL, NULL);
	CHECK_EQUAL(enc_sws, NULL, -1, "cannot alloc encoding swscale context")
	LOGD("1");

	enc_num = 0;

	dec_codec = avcodec_find_decoder(id);
	CHECK_EQUAL(dec_codec, NULL, -1, "decoding codec not found")
	LOGD("1");

	dec_ctx = avcodec_alloc_context3(dec_codec);
	CHECK_EQUAL(dec_ctx, NULL, -1, "cannot allocate decoding context")
	LOGD("1");

	dec_ctx->width = WIDTH;
	dec_ctx->height = HEIGHT;

	ret = avcodec_open2(dec_ctx, dec_codec, NULL);
	CHECK_LESS(ret, 0, -1, "cannot open decoding codec")
	LOGD("1");

	if (dec_codec->capabilities & CODEC_CAP_TRUNCATED)
		dec_ctx->flags |= CODEC_CAP_TRUNCATED;

	dec_frame = av_frame_alloc();
	CHECK_EQUAL(dec_frame, NULL, -1, "cannot alloc decoding frame")
	LOGD("1");

	rgb_frame = av_frame_alloc();
	CHECK_EQUAL(rgb_frame, NULL, -1, "cannot alloc decoding frame")
	ret = avpicture_fill((AVPicture *) rgb_frame,
	                     rgb_buffer, DISPLAY_FMT, WIDTH, HEIGHT);
	CHECK_LESS(ret, 0, -1, "cannot map rgb_buffer to decoding frame")
	LOGD("1");

	// dec_sws = sws_getCachedContext(dec_sws,
	//                                WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
	//                                WIDTH, HEIGHT, DISPLAY_FMT,
	//                                SWS_POINT, NULL, NULL, NULL);
	dec_sws = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_YUV420P,
	                         WIDTH, HEIGHT, DISPLAY_FMT,
	                         SWS_POINT, NULL, NULL, NULL);
	LOGD("1");

	enc_num = 0;
	dec_num = 0;

	return 0;
}

void coder_uninit()
{
	sws_freeContext(dec_sws);

	avcodec_close(dec_ctx);
	av_free(dec_ctx);

	av_frame_free(&rgb_frame);
	// av_frame_free(&dec_frame);

	sws_freeContext(enc_sws);

	avcodec_close(enc_ctx);
	av_free(enc_ctx);

	av_freep(&enc_frame->data[0]);
	av_frame_free(&yuyv_frame);

	LOGD("%s succeed", __func__);
}


int encode_frame()
{
	int ret, got_output = 0;

	av_init_packet(&enc_pkt);
	enc_pkt.data = NULL;
	enc_pkt.size = 0;

	enc_frame->pts = enc_num++;

	ret = avcodec_encode_video2(enc_ctx, &enc_pkt, enc_frame, &got_output);

	return ret < 0 ? ret : got_output;
}


int decode_frame()
{
	int ret, got_output = 0;

	ret = avcodec_decode_video2(dec_ctx, dec_frame, &got_output, &dec_pkt);
	if (ret < 0)
		return ret;

	if (dec_pkt.data)
	{
		dec_pkt.data += ret;
		dec_pkt.size -= ret;
	}

	return got_output;
}


/*	thread entry	*/

static unsigned long cap_num = 0;
void *capture_thread_entry(void *arg)
{
	LOGD("capture");
	int count = 0;

	int result;
	struct buffer *buf;

	if (v4l2_on() != 0)
		return NULL;

	clock_t begin = clock();
	cap_num = 0;

	while (video_on)
	{
		buf = v4l2_dqbuf();
		if (buf == NULL)
		{
			sleep(1);
			continue;
		}

		++cap_num;
		// LOGD("video frame %lu after capturing: %lu", cap_num, clock());
		memcpy(yuv_buffer, buf->start, buf->length);

		// clock_t begin = clock();
		sws_scale(enc_sws, (const uint8_t * const *) yuyv_frame->data, yuyv_frame->linesize,
		          0, CAPTURE_HEIGHT, enc_frame->data, enc_frame->linesize);
		// LOGD("sws_scale() = %.0f ms", (clock() - begin) * 1000.0 / CLOCKS_PER_SEC);

		// if (count++ < 30)
		// {
		// 	if (yuyv_fd > 0)
		// 		write(yuyv_fd, buf->start, buf->length);
		// }

		result = encode_frame();

		if (result <= 0)
			continue;

		send_video_frame(enc_pkt.data, enc_pkt.size);
		v4l2_qbuf();

		LOGD("one capture");
	}

	LOGD("%lu frames captured, used %.0f ms", cap_num, (clock() - begin) * 1000.0 / CLOCKS_PER_SEC);

	v4l2_off();

	return NULL;
}

static unsigned long dis_num = 0;
void *display_thread_entry(void *arg) 
{
	LOGD("display");
	int result;
	JNIEnv *env = attach_thread();

	clock_t begin = clock();
	dis_num = 0;

	while (video_on)
	{
		dec_pkt.size = read_video_frame(dec_buffer, CAPTURE_YUYV_SIZE);
		dec_pkt.data = dec_buffer;

		result = decode_frame();
		dis_num++;

		if (result <= 0)
			continue;

		sws_scale(dec_sws, (const uint8_t * const *) dec_frame->data, dec_frame->linesize,
		          0, HEIGHT, rgb_frame->data, rgb_frame->linesize);


		show_image(env, (int *) rgb_buffer, WIDTH * HEIGHT, WIDTH, HEIGHT);

		LOGD("one display");

		// LOGD("video frame %lu after displaying: %lu", ++dis_num, clock());

	}

	LOGD("%lu frames displayed, used %.0f ms", dis_num, (clock() - begin) * 1000.0 / CLOCKS_PER_SEC);

	detach_thread();

	return NULL;
}


/*  external calls  */

int start_video(int capture, int display)
{
	int res;

	if (coder_init(CODEC_ID_MPEG4) != 0)
		return -1;

	video_on = 1;
	if (capture)
	{
		fd = open(DEV_PATH, O_RDWR);
		LOGD("device file fd = %d", fd);
		if (fd < 0)
		{
			LOGD("open device file failed, errno = %d, error = %s", errno, strerror(errno));
			return -1;
		}

		if (init_device() != 0)
			return -1;

		res = pthread_create(&capture_thread, NULL, capture_thread_entry, NULL);
		if (res != 0)
		{
			LOGD("create capture_thread failed");
			return -1;
		}
	}

	if (display)
	{
		res = pthread_create(&display_thread, NULL, display_thread_entry, NULL);
		if (res != 0)
		{
			LOGD("create display_thread failed");
			return -1;
		}
	}

	return 0;
}

void stop_video()
{
	video_on = 0;

	if (display_thread != INVALID_PID)
	{
		pthread_join(display_thread, NULL);
		display_thread = INVALID_PID;
	}

	if (capture_thread != INVALID_PID)
	{
		pthread_join(capture_thread, NULL);
		uninit_device();
		close(fd);
		capture_thread = INVALID_PID;
	}

	coder_uninit();
}