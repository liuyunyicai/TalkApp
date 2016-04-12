#ifndef CIRCULAR_QUEUE
#define CIRCULAR_QUEUE

struct circular_queue;

typedef struct circular_queue circular_queue;

circular_queue *cq_create(int size);

int cq_read(circular_queue *cq, void *out, int max_size);

int cq_write(circular_queue *cq, const void *in, int size);

void cq_destroy(circular_queue *cq);

#endif