#ifndef __bufferlist_h__
#define __bufferlist_h__

#include <stdint.h>
#include <pthread.h>
#include <errno.h>

#include "logger.h"

typedef struct _bufferlist {
	uint32_t total;
	uint32_t used;
	uint32_t buffersize;
	uint32_t itemsize;
	struct _buffer* data;
} bufferlist_t;

typedef struct _buffer {
	uint32_t index;
	struct _buffer* next;
	struct _buffer* prev;
	struct _bufferlist* list;
	uint32_t count;
	void* data;
	pthread_mutex_t lock;
} buffer_t;

bufferlist_t* bufferlist_init(uint32_t num, uint32_t buffersize, uint32_t itemsize);
void bufferlist_destroy(bufferlist_t* list);
buffer_t* bufferlist_addbuffer(bufferlist_t* list, buffer_t* b);
void bufferlist_removebuffer(bufferlist_t* list, buffer_t* b);
int bufferlist_ratio(bufferlist_t* list);

buffer_t* buffer_checkout(bufferlist_t* list, buffer_t* b);
buffer_t* buffer_checkout_wait(bufferlist_t* list, buffer_t* b);

void buffer_checkin(bufferlist_t* list, buffer_t* b);

#endif
