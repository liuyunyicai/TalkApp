#include <time.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#include <android/log.h>

#include "circular_queue.h"

#define LOG_TAG		"BT_NATIVE_CIRCULAR_QUEUE"
#define LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_FUNC()	LOGD("current line: %d, function: %s", __LINE__, __func__)

typedef struct dynamic_array
{
	char *buf;
	int size;
	int capacity;

} dynamic_array;

typedef struct circular_queue
{
	int size;
	int rp;
	int wp;
	dynamic_array *data;

	sem_t sem;

} circular_queue;


/**
 * @brief Create a circular queue
 *
 * @param size The size of the circular queue
 * @return Returns a pointer of circular_queue if the creation succeeded or NULL if the creation failed
 */
circular_queue *cq_create(int size)
{
	circular_queue *cq;

	cq = (circular_queue *) calloc(1, sizeof (circular_queue));
	if (cq == NULL)
		return NULL;

	cq->size = size;
	cq->data = (dynamic_array *) calloc(size, sizeof (dynamic_array));
	if (cq->data == NULL)
	{
		free(cq);
		return NULL;
	}

	sem_init(&cq->sem, 0, 0);

	return cq;
}


/**
 * @brief Read a frame from the circular queue
 *
 * @param cq Pointer to a circular queue
 * @param out Buffer to save the output
 * @param max_size The length of out
 * @return Returns the bytes written into out or -1 if failed
 */
int cq_read(circular_queue *cq, void *out, int max_size)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;

	if (sem_timedwait(&cq->sem, &ts) != 0)
	{
		if (errno == ETIMEDOUT)
		{
			LOGD("semaphore wait timeout");
		}
		return -1;
	}

	int rp = cq->rp;
	dynamic_array *da = &cq->data[rp];

	if (da->size > max_size)
	{
		LOGD("no enough room for output, da->size = %d, max_size = %d", da->size, max_size);
		sem_post(&cq->sem);
		return -1;
	}

	int read = da->size;
	memcpy(out, da->buf, read);
	if (++cq->rp == cq->size)
		cq->rp = 0;

	return read;
}


/**
 * @brief Write a frame to the circular queue
 *
 * @param cq Pointer to a circular queue
 * @param in Buffer of the input
 * @param size The length of the input data
 * @return Returns the actual bytes written into the circular queue or -1 if failed
 */
int cq_write(circular_queue *cq, const void *in, int size)
{
	int wp = cq->wp;

	if ((cq->rp + cq->size - wp) % cq->size == 1)
	{
		LOGD("queue is full");
		return -1;
	}

	dynamic_array *da = &cq->data[wp];

	if (da->buf == NULL || da->capacity < size)
	{
		if (da->buf != NULL)
			free(da->buf);

		da->buf = (char *) malloc(size);
		if (da->buf == NULL)
		{
			LOGD("malloc memory for dynamic array failed");
			return -1;
		}
		da->capacity = size;
	}

	memcpy(da->buf, in, size);
	da->size = size;
	if (++cq->wp == cq->size)
		cq->wp = 0;

	sem_post(&cq->sem);

	return size;
}


/**
 * @brief Destroy the circular queue and release its resouces
 *
 * @param cq The circular queue to be destroyed
 */
void cq_destroy(circular_queue *cq)
{
	if (cq == NULL)
		return;

	int i;
	for (i = 0; i < cq->size; i++)
		free(cq->data[i].buf);
	free(cq->data);
	sem_destroy(&cq->sem);
	free(cq);
}
