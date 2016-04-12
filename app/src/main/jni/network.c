#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <android/log.h>

#include "network.h"
#include "circular_queue.h"


#define LOG_TAG		"BT_NATIVE_NETWORK"
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()	LOGD("current line: %d, function: %s", __LINE__, __func__)


#define	TCP_PORT	9000
#define QUEUE_SIZE	32

#define BUFFER_SIZE 820000


static int sockfd;
static int network_on;

static unsigned char frame_id;

static pthread_t recv_thread;

static circular_queue *audio_cq;
static circular_queue *video_cq;

static stream_head_t video_sh;
static stream_head_t audio_sh;

static codec_head_t video_ch;
static codec_head_t audio_ch;

static pthread_mutex_t write_lock;

static unsigned char recv_buffer[BUFFER_SIZE];
static unsigned char send_buffer[BUFFER_SIZE];


static char server_ip[128];


int read_n(int fd, void *buf, int n)
{
	if (buf == NULL)
	{
		LOGD("buf is NULL in %s", __func__);
		return -1;
	}

	char *ptr = buf;
	int bytes, remain = n;

	while (remain > 0)
	{
		if ((bytes = read(fd, ptr, remain)) < 0)
		{
			LOGD("errno = %d, msg = %s", errno, strerror(errno));
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		else if (bytes == 0)
		{
			break;
		}

		remain -= bytes;
		ptr += bytes;
	}

	return n - remain;
}

int write_n(int fd, void *buf, int n)
{
	if (buf == NULL)
	{
		LOGD("buf is NULL in %s", __func__);
		return -1;
	}

	char *ptr = buf;
	int bytes, remain = n;

	while (remain > 0)
	{
		if ((bytes = write(fd, ptr, remain)) < 0)
		{
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		else if (bytes == 0)
		{
			break;
		}

		remain -= bytes;
		ptr += bytes;
	}

	return n - remain;
}

unsigned long sync_end_code = SYN_END_CODE;

int send_frame(pstream_head_t psh, pcodec_head_t pch, const void *buffer, int size)
{
	unsigned char *ptr = send_buffer;

	memcpy(ptr, psh, sizeof (*psh));
	ptr += sizeof (*psh);

	memcpy(ptr, pch, sizeof (*pch));
	ptr += sizeof (*pch);

	memcpy(ptr, buffer, size);
	ptr += size;

	memcpy(ptr, &sync_end_code, sizeof (sync_end_code));
	ptr += sizeof (sync_end_code);

	// if (psh == &video_sh)
	// {
	// 	LOGD("video frame %lu before sending: %lu", video_ch.serial_num, clock());
	// }
	// else
	// {
	// 	LOGD("audio frame %lu before sending: %lu", audio_ch.serial_num, clock());
	// }

	return write_n(sockfd, send_buffer, ptr - send_buffer);
}


void *recv_thread_entry(void *arg)
{
	int bytes;

	stream_head_t sh;
	codec_head_t ch;
	circular_queue *cq = NULL;

	while (network_on)
	{
		 LOGD("begin read");
		bytes = read_n(sockfd, &sh, sizeof (sh));
		 LOGD("read stream head");
		if (bytes != sizeof (sh))
			break;

		bytes = read_n(sockfd, &ch, sizeof (ch));
		// LOGD("read codec head");
		if (bytes != sizeof (ch))
			break;

		bytes = read_n(sockfd, recv_buffer, ch.frame_size);
		// LOGD("read data");
		if (bytes != ch.frame_size)
			break;

		if (sh.msg_type == AUDIO_TYPE)
			cq = audio_cq;
		else
			cq = video_cq;

		cq_write(cq, recv_buffer, ch.frame_size - 4);

		// if (cq == video_cq)
		// {
		// 	LOGD("video frame %lu after recieving: %lu", ch.serial_num, clock());
		// }
		// else
		// {
		// 	LOGD("audio frame %lu after recieving: %lu", ch.serial_num, clock());
		// }
	}

	return NULL;
}


int network_start()
{
	audio_cq = cq_create(QUEUE_SIZE);
	video_cq = cq_create(QUEUE_SIZE);

	memset(&video_sh, 0, sizeof (video_sh));
	video_sh.align = ALIGN_HEAD;
	video_sh.msg_type = VIDEO_TYPE;

	memset(&audio_sh, 0, sizeof (audio_ch));
	audio_sh.align = ALIGN_HEAD;
	audio_sh.msg_type = AUDIO_TYPE;
	audio_sh.frame_type = AFrame;

	memset(&video_ch, 0, sizeof (video_ch));
	video_ch.syncode = SYN_START_CODE;
	video_ch.stream_type = VIDEO_FRAME;
	video_ch.codec_info = MPG4_ENCODE;

	memset(&audio_ch, 0, sizeof (audio_ch));
	audio_ch.syncode = SYN_START_CODE;
	audio_ch.width = 0;
	audio_ch.height = 0;
	audio_ch.stream_type = AUDIO_FRAME;
	audio_ch.frame_type = 0;
	audio_ch.codec_info = ADPCM_ENCODE;

	frame_id = 0;

	pthread_mutex_init(&write_lock, NULL);

	network_on = 1;
	pthread_create(&recv_thread, NULL, recv_thread_entry, NULL);
}

void network_stop()
{
	network_on = 0;
	pthread_join(recv_thread, NULL);
	LOGD("thread joined");

	pthread_mutex_destroy(&write_lock);
	LOGD("mutex destroyed");

	cq_destroy(video_cq);
	cq_destroy(audio_cq);
}

/*****	External API Calls	******/


int wait_for_client()
{
	int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_sockfd < 0)
	{
		LOGD("JNI LOG Network === cannot create server_sockfd: %d", server_sockfd);
		return -1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TCP_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	int server_len = sizeof (server_addr);

	int reuse = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (int)) < 0)
	{
		LOGD("JNI LOG Network === setsockopt error: %d, %s", errno, strerror(errno));
		return -1;
	}

	if (bind(server_sockfd, (struct sockaddr *) &server_addr, server_len))
	{
		LOGD("JNI LOG Network === bind socket error: %d, %s", errno, strerror(errno));
		return -1;
	}

	listen(server_sockfd, 5);
	LOGD("JNI LOG Network === waiting for client");

	struct sockaddr_in client_addr;
	int client_len = sizeof (client_addr);

	sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);
	LOGD("JNI LOG Network === client connected");

	network_start();

	close(server_sockfd);

	return 0;
}

int start_client()
{
	int server_len;
	struct sockaddr_in server_addr;

	LOGD("JNI NETWORK == start Client");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TCP_PORT);
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_len = sizeof (server_addr);

	LOGD("JNI NETWORK == connecting server, IP is %s", server_ip);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		LOGD("JNI NETWORK == cannot create sockfd: %d", sockfd);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *) &server_addr, server_len))
	{
		LOGD("JNI NETWORK == cannot connect to server, errno = %d, error = %s", errno, strerror(errno));
		return -1;
	}

	LOGD("JNI NETWORK == server connected");

	network_start();

	return 0;
}

void stop_network()
{
	// close(sockfd);
	shutdown(sockfd, SHUT_RD);
	LOGD("sockfd closed");
	network_stop();
}

int send_frame1(pstream_head_t psh, pcodec_head_t pch, const void *buffer, int size)
{
	if (psh->msg_type == AUDIO_TYPE)
	{
		cq_write(audio_cq, buffer, size);
	}
	else
	{
		cq_write(video_cq, buffer, size);
	}
}

int send_audio_frame(const void *data, int size)
{
	pthread_mutex_lock(&write_lock);

	audio_sh.rsv[0] = frame_id++;
	if (frame_id == (unsigned char) 255)
		frame_id = 0;
	audio_sh.size = sizeof (codec_head_t) + size + 4;

	audio_ch.time_stamp = 0;
	audio_ch.serial_num++;
	audio_ch.frame_size = size + 4;							//	including SYN_END_CODE

	int bytes = send_frame(&audio_sh, &audio_ch, data, size);

	pthread_mutex_unlock(&write_lock);

	return bytes;
}


int send_video_frame(const void *data, int size)
{
	pthread_mutex_lock(&write_lock);

	// video_sh.frame_type =
	video_sh.rsv[0] = frame_id++;
	if (frame_id == (unsigned char) 255)
		frame_id = 0;
	video_sh.size = sizeof (codec_head_t) + size + 4;

	// video_ch.width =
	// video_ch.height =
	// video_ch.frame_type =
	// video_ch.time_stamp =
	video_ch.serial_num++;
	video_ch.frame_size = size + 4;

	int bytes = send_frame(&video_sh, &video_ch, data, size);

	pthread_mutex_unlock(&write_lock);

	return bytes;
}

int read_video_frame(void *data, int max_size)
{
	return cq_read(video_cq, data, max_size);
}

int read_audio_frame(void *data, int max_size)
{
	return cq_read(audio_cq, data, max_size);
}

void set_server_ip(const char *ip)
{
	strcpy(server_ip, ip);
}
