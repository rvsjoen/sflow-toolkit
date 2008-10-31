#ifndef __bufferqueue_h__
#define __bufferqueue_h__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <util.h>
#include "logger.h"

typedef struct _bufferqueue {
	uint32_t num;
	uint32_t buffersize;
	uint32_t itemsize;
	struct _buffer* start;
	struct _buffer* end;
	pthread_mutex_t lock;
	pthread_cond_t condition;
} bqueue_t;

typedef struct _buffer {
	uint32_t index;
	struct _buffer* next;
	struct _buffer* prev;
	uint32_t count;
	void* data;
} buffer_t;

bqueue_t* bqueue_init(uint32_t num, uint32_t buffersize, uint32_t itemsize);
void bqueue_destroy(bqueue_t*);
void bqueue_push(bqueue_t* queue, buffer_t* b);
buffer_t* bqueue_pop(bqueue_t* queue);
buffer_t* bqueue_pop_wait(bqueue_t* queue);
int bqueue_push_new(bqueue_t* queue);
void bqueue_free(bqueue_t* buf);

#endif
