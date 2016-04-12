#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include <stdlib.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"

#include "unistd.h"
#include "fcntl.h"
#include "errno.h"
#include "math.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "sys/mman.h"
#include "errno.h"
#include "time.h"
#include "sys/poll.h"

#include "pthread.h"
#include "jni.h"
#include "linux/videodev2.h"

#include "jni_network.h"

#define CAPTURE_BUFFER_NUMBER 5
#define WID 352
#define HEIG 288
#define LEN (WID * HEIG * 3 / 2)
#define RGB32LEN (WID * HEIG * 4)

const int save_4WID = WID * 4;
const int save_WID2 = WID / 2;
const int WH = WID * HEIG;

#define LOG_TAG		"jni_video"
#define CLEAR(x)	memset(&(x), 0, sizeof(x))
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void *capture_thread(void *arg);
void *decode_thread(void *arg);

typedef struct tagBITMAPFILEHEADER{
	short bfType;
	int bfSize;
	short bfReserved1;
	short bfReserved2;
	int bfOffBits;
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	int biSize;
	int biWidth;
	int biHeight;
	short biPlanes;
	short biBitCount;
	int biCompression;
	int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	int biClrUsed;
	int biClrImportant;
}BITMAPINFOHEADER;

struct buffer {
	void * start;
	int length;
};

//capture related
int fd;
static int capture_num = 0, decode_num = 0;
struct buffer buffers[CAPTURE_BUFFER_NUMBER];
int n_buffers= 0;
uint8_t capture_data[8 * WID * HEIG];

//thread related
struct video_buf {
	int length;
	uint8_t data[LEN];
};

pthread_t RecordVideoThread, DecodeVideoThread;
void *record_result, *decode_result;
int videoOn;

//android API related
JavaVM *g_jvm = NULL;
jclass tiny_class = NULL, bitmap_class = NULL, bitmap_config_class = NULL;
jfieldID myHolder_field = NULL, rgb32_field = NULL;
jobject myHolder_obj = NULL, paint_obj = NULL, Rect_obj = NULL, rgb32_obj = NULL;
jmethodID mLockCanvas = NULL, mUnlockCanvasAndPost = NULL, mCreateBitmap = NULL, 
	mRecycle = NULL, mDrawBitmap = NULL;
jintArray dataArray = NULL;

//ffmpeg related
static AVCodec *codec_encode = NULL, *codec_decode = NULL;
static AVCodecContext *c_encode= NULL, *c_decode = NULL;

AVPacket pkt_record, pkt_decode;
AVFrame *frame_encode = NULL, *frame_decode = NULL, *frameRGB_decode = NULL;
uint8_t rgb_buffer[RGB32LEN];
int rgb_color[RGB32LEN / 4];

struct SwsContext *sws_ctx;

int start_video_module(JNIEnv *env, jobject obj)
{
	
	fd = open("/dev/video0", O_RDWR);
	if(fd == -1)
		LOGD("open camera failed!!!");
	else
		LOGD("fd = %d, open camera succeeded!!!", fd);

	if(init_all(env, obj) == 0)
		LOGD("initAll succeeded!!");
	else
		LOGD("initAll failed!!");

	return 0;
	
}

int stop_video_module()
{

	if(finish_all() == 0)
		LOGD("finish_all succeed!!");
	else
		LOGD("finish_all failed!!");
	
}

int init_all(JNIEnv *env, jobject obj)
{

	if(init_image(env, obj) == 0)
		LOGD("init_image succeeded!!");
	else
	{
		LOGD("init_image failed!!");
		return -1;
	}

	if(video_encode_init(AV_CODEC_ID_MPEG4) == 0)
		LOGD("video_encode_init succeeded!!");
	else
	{
		LOGD("video_encode_init failed!!");
		return -1;
	}
	
	if(init_device() == 0)
		LOGD("init_device succeeded!!");
	else
	{
		LOGD("init_device failed!!");
		return -1;
	}

	if(init_mmap() == 0)
		LOGD("init_mmap succeeded!!");
	else
	{
		LOGD("init_mmap failed!!");
		return -1;
	}

	if(init_thread() == 0)
		LOGD("init_thread succeeded!!");
	else
	{
		LOGD("init_thread failed!!");
		return -1;
	}

	return 0;
	
}

int init_image(JNIEnv *env, jobject obj)
{

	(*env)->GetJavaVM(env, &g_jvm);
	
	tiny_class = (*env)->GetObjectClass(env, obj);
	if(tiny_class == NULL) {
		LOGD("tiny_class == NULL");
		return -1;
	}
	tiny_class = (*env)->NewGlobalRef(env, tiny_class);
	LOGD("Get tiny_class succeeded!!!");
	
	myHolder_field = (*env)->GetFieldID(env, tiny_class, "myHolder", "Landroid/view/SurfaceHolder;");
	if(myHolder_field == NULL) {
		LOGD("Get SurfaceHolder fieldID or get myHolder object failed!!!");
		return -1;
	}
	myHolder_obj = (*env)->GetObjectField(env, obj, myHolder_field);
	if(myHolder_obj == NULL) {
		LOGD("Get SurfaceHolder fieldID or get myHolder object failed!!!");
		return -1;
	}
	myHolder_obj = (*env)->NewGlobalRef(env, myHolder_obj);
	LOGD("Get myHolder succeed!!");

	jclass SurfaceHolder_class = (*env)->FindClass(env, "android/view/SurfaceHolder");
	if(SurfaceHolder_class == NULL) {
		LOGD("Find SurfaceHolder class failed!!!");
		return -1;
	}
	LOGD("Get SurfaceHolder class!!!");
	
	mLockCanvas = (*env)->GetMethodID(env, SurfaceHolder_class, 
			"lockCanvas", "(Landroid/graphics/Rect;)Landroid/graphics/Canvas;");
	mUnlockCanvasAndPost = (*env)->GetMethodID(env, SurfaceHolder_class, 
			"unlockCanvasAndPost", "(Landroid/graphics/Canvas;)V");
	if((mLockCanvas == NULL) || (mUnlockCanvasAndPost == NULL)) {
		LOGD("Get Method lockCanvas orunlockCanvasAndPost failed!!!");
		return -1;
	}
	LOGD("Get mLockCanvas and mUnlockCanvasAndPost!!!");

	jclass Rect_class = (*env)->FindClass(env, "android/graphics/Rect");
	if(Rect_class == NULL) {
		LOGD("Find class Rect failed!!!");
		return -1;
	}
	jmethodID mRect = (*env)->GetMethodID(env, Rect_class, "<init>", "(IIII)V");
	Rect_obj = (*env)->NewObject(env, Rect_class, mRect, 0, 0, WID, HEIG);
	if((mRect == NULL) || (Rect_obj == NULL)) {
		LOGD("New Object failed!!!");
		return -1;
	}
	Rect_obj = (*env)->NewGlobalRef(env, Rect_obj);
	LOGD("Get Rect obj!!!");

	bitmap_class = (*env)->FindClass(env, "android/graphics/Bitmap");
	bitmap_class = (*env)->NewGlobalRef(env, bitmap_class);
	if(bitmap_class == NULL) {
		LOGD("Find Bitmap class failed!!!");
		return -1;
	}
	LOGD("Get Bitmap class!!!");

	mCreateBitmap = (*env)->GetStaticMethodID(env, bitmap_class, "createBitmap",
		"([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
	if(mCreateBitmap == NULL) {
		LOGD("Find CreateBitmap method failed!!!");
		return -1;
	}
	LOGD("Get CreateBitmap method!!!");

	bitmap_config_class = (*env)->FindClass(env, "android/graphics/Bitmap$Config");
	if(bitmap_config_class == NULL) {
		LOGD("Find bitmap_config_class class failed!!!");
		return -1;
	}
	LOGD("Get bitmap_config_class class!!!");

	rgb32_field =  (*env)->GetStaticFieldID(env, bitmap_config_class,
				"ARGB_8888", "Landroid/graphics/Bitmap$Config;");
	if(rgb32_field == NULL) {
		LOGD("Find rgb32_field failed!!!");
		return -1;
	}
	LOGD("Get rgb32_field!!!");

	rgb32_obj = (*env)->GetStaticObjectField(env, bitmap_config_class, rgb32_field);
	if(rgb32_obj == NULL) {
		LOGD("Find rgb32_obj failed!!!");
		return -1;
	}
	
	mRecycle = (*env)->GetMethodID(env, bitmap_class, "recycle", "()V");
	if(mRecycle == NULL) {
		LOGD("Get recycle method failed!!!");
		return -1;
	}
	LOGD("Get recycle method!!!");

	jclass paint_class = (*env)->FindClass(env, "android/graphics/Paint");
	if(paint_class == NULL) {
		LOGD("find paint class failed!!!");
		return -1;
	}
	jmethodID mPaint = (*env)->GetMethodID(env, paint_class, "<init>", "()V");
	paint_obj = (*env)->NewObject(env, paint_class, mPaint);
	paint_obj = (*env)->NewGlobalRef(env, paint_obj);
	if((mPaint == NULL) || (paint_obj == NULL)) {
		LOGD("GetMethodID or NewObject failed!!!!");
		return -1;
	}
	LOGD("Get paint obj !!!");

	jclass canvas_class = (*env)->FindClass(env, "android/graphics/Canvas");
	if(canvas_class == NULL) {
		LOGD("find canvas class failed!!!");
		return -1;
	}
	mDrawBitmap = (*env)->GetMethodID(env, canvas_class, 
			"drawBitmap", "(Landroid/graphics/Bitmap;FFLandroid/graphics/Paint;)V");
	if(mDrawBitmap == NULL) {
		LOGD("Get drawBitmap method failed!!!");
	}
	LOGD("Get mDrawBitmap!!!");	

	dataArray = (*env)->NewIntArray(env, RGB32LEN / 4);
	dataArray = (*env)->NewGlobalRef(env, dataArray);
	if(dataArray == NULL) {
		LOGD("New Int Array dataArray failed!!!");
	}
	LOGD("New Int Array dataArray succeeded!!!");

	(*env)->DeleteLocalRef(env, SurfaceHolder_class);
	(*env)->DeleteLocalRef(env, Rect_class);
	(*env)->DeleteLocalRef(env, paint_class);
	(*env)->DeleteLocalRef(env, canvas_class);
	(*env)->DeleteGlobalRef(env, tiny_class);
	tiny_class = NULL;
	
	LOGD("init Image finished!!!!");
	
	return 0;
	
}

int video_encode_init(int codec_id)
{

	int ret;
	avcodec_register_all();

	/* ----------------encoding--------------- */
    codec_encode = avcodec_find_encoder(codec_id);
    if (!codec_encode) {
        LOGD("encoder not found !!");
		return -1;
    }

	c_encode = avcodec_alloc_context3(codec_encode);
    if (!c_encode) {
		LOGD("Could not allocate video codec_encode context");
		return -1;
    }
	
	/* put sample parameters */
    c_encode->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c_encode->width = WID;
    c_encode->height = HEIG;
    /* frames per second */
    c_encode->time_base= (AVRational){1,15};
    c_encode->gop_size = 10; /* emit one intra frame every ten frames */
    c_encode->max_b_frames=1;
    c_encode->pix_fmt = AV_PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open2(c_encode, codec_encode, NULL) < 0) {
        LOGD("Could not open codec");
        return -1;
    }

	/* --------------decoding -------------- */
	codec_decode = avcodec_find_decoder(codec_id);
    if (!codec_decode) {
        LOGD("decoder not found !!");
		return -1;
    }

	c_decode = avcodec_alloc_context3(codec_decode);
    if (!c_decode) {
		LOGD("Could not allocate video codec_decode context");
		return -1;
    }
	
	/* we do not send complete frames, for decoding */
    if(codec_decode->capabilities & CODEC_CAP_TRUNCATED)
        c_decode->flags|= CODEC_FLAG_TRUNCATED; 
	
	if (avcodec_open2(c_decode, codec_decode, NULL) < 0) {
        LOGD("Could not open codec");
        return -1;
    }
	
	frame_encode = avcodec_alloc_frame();
    if (!frame_encode) {
        LOGD("Could not allocate video frame");
        return -1;
    }
    frame_encode->format = c_encode->pix_fmt;
    frame_encode->width  = WID;
    frame_encode->height = HEIG;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame_encode->data, frame_encode->linesize, WID, HEIG, AV_PIX_FMT_YUV420P, 32);
    if (ret < 0) {
        LOGD("Could not allocate raw picture buffer");
        return -1;
    }
	else
		LOGD("av_image_alloc succeeded!!");

	/* create scaling context */
    sws_ctx = sws_getContext(WID, HEIG, AV_PIX_FMT_YUV420P,
                             WID, HEIG, AV_PIX_FMT_RGB32,
                             SWS_BILINEAR, NULL, NULL, NULL);

    frame_decode = avcodec_alloc_frame();
    if (!frame_decode) {
        LOGD("Could not allocate video frame_decode");
        return -1;
    }

	frameRGB_decode= avcodec_alloc_frame();
	if (!frameRGB_decode) {
        LOGD("Could not allocate video frameRGB_decode");
        return -1;
    }

	int AVPicture_size = avpicture_fill( (AVPicture *)frameRGB_decode, rgb_buffer, 
									AV_PIX_FMT_RGB32, WID, HEIG);

	return 0;
	
}

int init_device()
{

	int ret;
	struct v4l2_capability cap;
	CLEAR(cap);
	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if(ret == 0)
		LOGD("VIDIOC_QUERYCAP succeeded!!");
	else
	{
		LOGD("ioctl cap failed!!!");
		return -1;
	}
	LOGD("driver = %s, card = %s, bus = %s, version = %d.%d, Capabilities = %08x", cap.driver, 
		cap.card, cap.bus_info, (cap.version >> 16) && 0xff, (cap.version >> 24) && 0xff, 
		cap.capabilities);

	if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)   //can capture image
		LOGD("Capture capability is supported");
	
	if (cap.capabilities & V4L2_CAP_STREAMING)   		//can dqbuf and qbuf
		LOGD("V4L2_CAP_STREAMING is supported");

	struct v4l2_input input;
	CLEAR(input);
	input.index = 0;
	ret = ioctl(fd, VIDIOC_ENUMINPUT, &input);
	if(ret == 0)
    	LOGD("VIDIOC_ENUMINPUT succeeded!!");
	else
		LOGD("VIDIOC_ENUMINPUT failed!!");
	
	ret = ioctl(fd, VIDIOC_S_INPUT, &input);
	if(ret == 0)
    	LOGD("VIDIOC_S_INPUT succeeded!!");
	else
		LOGD("VIDIOC_S_INPUT failed!!");

//	struct v4l2_fmtdesc fmtdes;
//	CLEAR(fmtdes);
//	fmtdes.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//	while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdes)) == 0)
//	{
//		LOGD("VIDIOC_ENUM_FMT index = %d", fmtdes.index);
//		LOGD("{ pixelformat = %c%c%c%c, description = %s }",
//				(fmtdes.pixelformat & 0xFF),
//				(fmtdes.pixelformat >> 8) & 0xFF,
//				(fmtdes.pixelformat >> 16) & 0xFF,
//				(fmtdes.pixelformat >> 24) & 0xFF,
//				fmtdes.description);

//		fmtdes.index++;
//		if(fmtdes.index >= 20)
//			break;
//	}
//	if(ret != 0)
//	{
//		LOGD("VIDIOC_ENUM_FMT failed!!");
//		return -1;
//	}

	struct v4l2_format fmt;
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = WID * 2;
	fmt.fmt.pix.height = HEIG * 2;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;	 //ffmpeg: yuv422p is invalid or not supported!!
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if(ret == 0)
		LOGD("VIDIOC_S_FMT succeeded!!!");
	else
		LOGD("VIDIOC_S_FMT failed!!!");

	LOGD("Selected Camera Mode:   Width: %d, Height: %d, PixFmt: %s, Field: %d, sizeimage = %d",
                 fmt.fmt.pix.width, fmt.fmt.pix.height, 
                 (char *)&fmt.fmt.pix.pixelformat, fmt.fmt.pix.field, fmt.fmt.pix.sizeimage);
	LOGD("bytesperline = %d", fmt.fmt.pix.bytesperline);

	return 0;
	
}

int init_mmap()
{

	int ret;
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = CAPTURE_BUFFER_NUMBER;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &req);
	if((ret == 0) && (req.count == CAPTURE_BUFFER_NUMBER))
		LOGD("VIDIOC_REQBUFS succeeded!!!");
	else
	{
		LOGD("VIDIOC_REQBUFS failed!!!");
		return -1;
	}
	LOGD("count = %d, type = %d, memory = %d", req.count, req.type, req.memory);

	for(n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{
	
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		LOGD("type = %d, memory = %d, index = %d", buf.type, buf.memory, buf.index);
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if(ret == 0)
			LOGD("VIDIOC_QUERYBUF succeeded!!!");
		else
		{
			LOGD("VIDIOC_QUERYBUF failed!!!");
			return -1;
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap (0, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if(buffers[n_buffers].start == NULL)
		{
			LOGD("buffers[%d].start == NULL", n_buffers);
			return -1;
		}
		LOGD("Length: %d, offset = %d, Address: %p", 
			buffers[n_buffers].length, buf.m.offset, buffers[n_buffers].start);
		
	}

	for(n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{

		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if(ret == 0)
			LOGD("VIDIOC_QBUF succeeded!!!");
		else
		{
			LOGD("VIDIOC_QBUF failed!!!");
			return -1;
		}
		
	}

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if(ret == 0)
		LOGD("VIDIOC_STREAMON succeeded!!!");
	else
	{
		LOGD("VIDIOC_STREAMON failed!!!");
		return -1;
	}
		
	return 0;
	
}

int init_thread()
{
	
	int ret;
	
	videoOn = 1;
	
	LOGD("RecordVideoThread");
	capture_num = 0;
	ret = pthread_create(&RecordVideoThread, NULL, capture_thread, NULL);
	if(ret != 0)
	{
		LOGD("record video Thread creation failed!!!");
		return -1;
	}

	// LOGD("DecodeVideoThread");
	// decode_num = 0;
	// ret = pthread_create(&DecodeVideoThread, NULL, decode_thread, NULL);
	// if(ret != 0)
	// {
	// 	LOGD("deocde video Thread creation failed!!!");
	// 	return -1;
	// }

	return 0;
	
}

void *capture_thread(void *arg)
{
	int ret;

	struct video_buf capture_buf;
	LOGD("capture_thread BEGIN!! capture_num = %d", capture_num);
	while(videoOn)
	{
		
		// LOGD("capture_thread :  num = %d", capture_num);
//		LOGD("capture time1 : %ld", clock());
		if((ret = capture_image(&capture_buf)) != 0)
		{
			LOGD("capture_image()=%d", ret);
			break;
		}

//		LOGD("capture time2 : %ld", clock());
		usleep(100000);
		int ret = sendVideoFrame(capture_buf.data, capture_buf.length, WID, HEIG);
		// send_video_frame(capture_buf.data, capture_buf.length);
		LOGD("sendVideoFrame()=%d", ret);
//		LOGD("capture time3 : %ld", clock());
		
	}

	pthread_exit(NULL);
	
}

int capture_image(struct video_buf *pcapture_buf)
{

	int ret ;
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
//	if(ret == 0)
//		LOGD("%d : VIDIOC_DQBUF succeeded!!!", capture_num);
//	else
//		LOGD("VIDIOC_DQBUF failed!!!");
	int index = buf.index;

//	LOGD("buf.index = %d, byteused = %d, offset = %d", buf.index, buf.bytesused, buf.m.offset);

//		pgm_save(buffers[buf.index].start, LEN, WID, HEIG, capture_num);
//		fwrite(buffers[index].start, 1, buffers[index].length, outf_yuv);

	memcpy(capture_data, buffers[index].start, buffers[index].length);
	ret = process_image(capture_data, buffers[index].length, pcapture_buf);

	if(ret == 0)
		LOGD("process_image succeeded!!");
	else
		LOGD("process_image failed!!");

	capture_num++;
	if(capture_num >= (1 << 30))
		capture_num = 0;

	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;
	ret = ioctl(fd, VIDIOC_QBUF, &buf);
//	if(ret == 0)
//		LOGD("VIDIOC_QBUF succeeded!!!");
//	else
//	{
//		LOGD("VIDIOC_QBUF failed!!!");
//		return -1;
//	}
		
	return 0;
	
}

int process_image(uint8_t *data, int len, struct video_buf *pcapture_buf) 	//capture_num for frame->pts
{

	int i, j, got_output, ret;
	int h, w, m, n, k, x, y, z;
	int save_lineh0, save_lineh1, save_lineh2;
    av_init_packet(&pkt_record);
    pkt_record.data = NULL;    // packet data will be allocated by the encoder
    pkt_record.size = 0;

	x = 0, y = 1, z = 3;
	for(h = 0; h < HEIG; h++)
	{
	
		m = 0, n = 0, k = 0;
		save_lineh0 = h * frame_encode->linesize[0];
		save_lineh1 = h / 2 * frame_encode->linesize[1];
		save_lineh2 = h / 2 * frame_encode->linesize[2];
		
		for(w = 0; w < save_WID2; w++)
		{

			frame_encode->data[0][save_lineh0 + m] = data[x];
			x += 2;
			m++;
			frame_encode->data[0][save_lineh0 + m] = data[x];
			x += 6;
			m++;

			if(!(h & 1))
			{
				frame_encode->data[1][save_lineh1 + n] = 
					(data[y] + data[y + 4] + data[y + save_4WID] + data[y + save_4WID + 4]) / 4;
				n++;
				
				frame_encode->data[2][save_lineh2 + k] = 
					(data[z] + data[z + 4] + data[z + save_4WID] + data[z + save_4WID + 4]) / 4;
				k++;
			}

			y += 8;
			z += 8;
			
		}
		
		x += save_4WID;
		y += save_4WID;
		z += save_4WID;
		
	}
    frame_encode->pts = capture_num;

    /* encode the image */
//	LOGD("begin to encode the image!!");
    ret = avcodec_encode_video2(c_encode, &pkt_record, frame_encode, &got_output);
//    if (ret < 0) {
//        LOGD("Error encoding frame!!    got_output = %d, ret = %d", got_output, ret);
//        return -1;
//    }
//	else
//		LOGD("avcodec_encode_video2 succeeded!!!   got_output = %d, size = %d", 
//						got_output, pkt_record.size);

    if (got_output) 
	{
		
		pcapture_buf->length = pkt_record.size;
		memcpy(pcapture_buf->data, pkt_record.data, pkt_record.size);
		
    }

	av_free_packet(&pkt_record);

	return 0;
	
}

void *decode_thread(void *arg)
{

	JNIEnv *env;
	if((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != JNI_OK)
	{
		LOGD("AttachCurrentThread   Video failed!!!");
		pthread_exit(NULL);
	}
	
	struct video_buf decode_buf;
	av_init_packet(&pkt_decode);

    while(videoOn)
	{

		// LOGD("decode time1 : %ld", clock());

		pkt_decode.size = getVideoFrame(decode_buf.data, LEN);
		// LOGD("getVideoFrame()=%d", pkt_decode.size);
		// pkt_decode.size = get_video_frame(decode_buf.data);
		pkt_decode.data = decode_buf.data;
		
		if(pkt_decode.size < 0)
		{
			usleep(5000);
			continue;
		}
		LOGD("decode_thread :  num = %d", decode_num);
		
        while (pkt_decode.size > 0)
        {
            if (decode_write_frame(env) < 0)
                break;
        }

		LOGD("decode time2 : %ld", clock());

    }
	LOGD("why nono!");
	
	av_free_packet(&pkt_decode);
	
	if((*g_jvm)->DetachCurrentThread(g_jvm) != JNI_OK)
		LOGD("DetachCurrentThread() buf failed!!!!");
	pthread_exit(NULL);
	
}

int decode_write_frame(JNIEnv *env)
{
	
    int len, got_frame;
//	LOGD("pkt.size = %d", pkt_decode.size);
    len = avcodec_decode_video2(c_decode, frame_decode, &got_frame, &pkt_decode);
    if (len < 0) {
        LOGD("Error while decoding frame!!   len = %d", len);
        return len;
    }
//	LOGD("len = %d, got_frame = %d", len, got_frame);
	
    if (got_frame) {

		/* save format : RGB24 */
		sws_scale(sws_ctx, (const uint8_t* const*)frame_decode->data, frame_decode->linesize, 0, 
						HEIG, frameRGB_decode->data, frameRGB_decode->linesize);

		if(show_image(env, rgb_buffer) < 0)
		{
			LOGD("SaveFrame failed!!");
			return -1;
		}
//		LOGD("SaveFrame succeeded!!");

		decode_num++;
		if(decode_num >= (1 << 30))
			decode_num = 0;
		 
    }

	if(pkt_decode.data)
	{
		pkt_decode.data += len;
		pkt_decode.size -= len;
	}

	return 0;
	
}

int show_image(JNIEnv *env, uint8_t *buf)
{
	
	int i, j, temp1, temp2, temp3, temp4;
	
	jobject canvas_obj = (*env)->CallObjectMethod(env, myHolder_obj, mLockCanvas, Rect_obj);
	if(canvas_obj == NULL)
	{
		LOGD("call lockCanvas failed!!!");
		return -1;
	}
//	LOGD("Call Object Method succeed!!!");

	for(i = 0; i < RGB32LEN /4; i++)
	{

		temp1 = (buf[4 * i]) & 0x000000ff;
		temp2 = (buf[4 * i + 1] << 8) & 0x0000ff00;
		temp3 = (buf[4 * i + 2] << 16) & 0x00ff0000;
		temp4 = (buf[4 * i + 3] << 24) & 0xff000000;
		
		rgb_color[i] = temp1 | temp2 | temp3 | temp4;
		
	}

	(*env)->SetIntArrayRegion(env, dataArray, 0, RGB32LEN / 4, (jint *)rgb_color);
	
	jobject bitmap_obj = (*env)->CallStaticObjectMethod(env, bitmap_class, 
								mCreateBitmap, dataArray, WID, HEIG, rgb32_obj);
	if(bitmap_obj == NULL)
	{
		LOGD("bitmap_obj is NULL!!!");
		return 0;
	}
//	LOGD("Call static object method succeed!!!");

	(*env)->CallVoidMethod(env, canvas_obj, mDrawBitmap, bitmap_obj, 0.0, 0.0, paint_obj);
	(*env)->CallVoidMethod(env, myHolder_obj, mUnlockCanvasAndPost, canvas_obj);
	(*env)->CallVoidMethod(env, bitmap_obj, mRecycle);
	
	(*env)->DeleteLocalRef(env, bitmap_obj);
	(*env)->DeleteLocalRef(env, canvas_obj);
			
	return 0;
	
}

int finish_all()
{

	if(free_thread() == 0)
		LOGD("free_thread succeeded!!");
	else
	{
		LOGD("free_thread failed!!");
		return -1;
	}
	
	if(v4l2_off() == 0)
		LOGD("v4l2_off succeeded!!");
	else
	{
		LOGD("v4l2_off failed!!");
		return -1;
	}

	if(record_finish() == 0)
		LOGD("record_finish succeeded!!");
	else
	{
		LOGD("record_finish failed!!");
		return -1;
	}
	
	return 0;
	
}

int free_thread()
{

	videoOn = 0;
 	int ret;
	ret = pthread_join(RecordVideoThread, &record_result);
	if(ret != 0)
	{
		LOGD("RecordVideoThread join failed!!");
		return -1;
	}
	else
		LOGD("RecordVideoThread join succeeded!!");

	// ret = pthread_join(DecodeVideoThread, &decode_result);
	// if(ret != 0)
	// {
	// 	LOGD("DecodeVideoThread join failed!!");
	// 	return -1;
	// }
	// else
	// 	LOGD("DecodeVideoThread join succeeded!!");

	return 0;
	
}

int v4l2_off()
{

	int ret;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if(ret == 0)
		LOGD("VIDIOC_STREAMOFF succeeded!!!");	
	else
	{
		LOGD("VIDIOC_STREAMOFF failed!!!");
		return -1;
	}

	close(fd);
	fd = -1;
	
	for(n_buffers = 0; n_buffers < CAPTURE_BUFFER_NUMBER; n_buffers++)
	{
		munmap(buffers[n_buffers].start, buffers[n_buffers].length);
	}	

	return 0;

}

int record_finish()
{

	JNIEnv *env;
	if((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != JNI_OK)
	{
		LOGD("AttachCurrentThread   Video failed!!!");
		return -1;
	}

	(*env)->DeleteGlobalRef(env, myHolder_obj);
	(*env)->DeleteGlobalRef(env, Rect_obj);
	(*env)->DeleteGlobalRef(env, bitmap_class);
	(*env)->DeleteGlobalRef(env, paint_obj);
	(*env)->DeleteGlobalRef(env, dataArray);

	myHolder_field = NULL;
	mLockCanvas = NULL;
	mUnlockCanvasAndPost = NULL;
	mRecycle = NULL;
	mDrawBitmap = NULL;
	
	sws_freeContext(sws_ctx);
	
    avcodec_free_frame(&frame_decode);
	avcodec_free_frame(&frameRGB_decode);

    av_freep(&frame_encode->data[0]);
    avcodec_free_frame(&frame_encode);

    avcodec_close(c_encode);
	avcodec_close(c_decode);
    av_free(c_encode);
	av_free(c_decode);
	
	return 0;
	
}




