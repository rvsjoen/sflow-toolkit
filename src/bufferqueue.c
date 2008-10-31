#include "bufferqueue.h"

extern uint32_t num_buffers;

bqueue_t* bqueue_init(uint32_t num, uint32_t buffersize, uint32_t itemsize){
	logmsg(LOGLEVEL_DEBUG, "Initializing buffer queue");
	bqueue_t* b = (bqueue_t*) malloc(sizeof(bqueue_t));
	memset(b, 0, sizeof(bqueue_t));

	pthread_mutex_init(&(b->lock), NULL);
	pthread_cond_init(&(b->condition), NULL);

	b->buffersize = buffersize;
	b->itemsize = itemsize;

	logmsg(LOGLEVEL_DEBUG, "Pushing initial free buffers");
	uint32_t i;
	for( i=0; i<num; i++)
		bqueue_push_new(b);
	return b;
}

void bqueue_destroy(bqueue_t* b){
	logmsg(LOGLEVEL_DEBUG, "Destroying buffer");
	while(b->num > 0){
		buffer_t* buf = bqueue_pop_wait(b);
		free(buf->data);
		free(buf);
	}
	free(b);
}

void bqueue_free(bqueue_t* queue){
	if(queue->num > 0){
		buffer_t* b = queue->start;
		queue->start = b->next;
		queue->start->prev = NULL;
		free(b->data);
		free(b);
		queue->num--;
	}
}

void bqueue_push(bqueue_t* queue, buffer_t* b){
	pthread_mutex_t* lock = &(queue->lock);
	if(pthread_mutex_lock(lock) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_on_error();
	}
	if(queue->num == 0){
		queue->start = b;
		queue->end = b;
	} else {
		queue->start->prev = b;
		b->next = queue->start;
		b->prev = 0;
		queue->start = b;
	}
	queue->num++;
	if(pthread_mutex_unlock(lock) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_on_error();
	}
	pthread_cond_signal(&(queue->condition));
}

buffer_t* bqueue_pop(bqueue_t* queue){
	buffer_t* b = NULL;
	pthread_mutex_t* lock = &(queue->lock);

	if(pthread_mutex_lock(lock) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_on_error();
	}

	int res = 0;
	while(queue->num == 0){
		struct timespec t;
		t.tv_sec = 1;
		t.tv_nsec = 0;
		res = pthread_cond_timedwait(&(queue->condition), lock, &t);

	}

	// Nothing is happening, try to allocate a new buffer
	if(res == ETIMEDOUT){
		logmsg(LOGLEVEL_DEBUG, "Timed out waiting for new buffer, allocating a new one");
		// Try to allocate a buffer, if not possible we wait until something frees up
		if(bqueue_push_new(queue) == ENOMEM){
			while (queue->num == 0) {
				if(pthread_cond_wait(&(queue->condition), lock) == -1){
					logmsg(LOGLEVEL_ERROR, strerror(errno));
					exit_on_error();
				}
			}
		}
	} else if(queue->num > num_buffers){
		logmsg(LOGLEVEL_DEBUG, "Too many free buffers, freeing one");
		bqueue_free(queue);
	}

	if(queue->num == 1){
		b = queue->start;
		queue->start = NULL;
		queue->end = NULL;
	} else {
		b = queue->end;
		queue->end->prev->next = NULL;
		queue->end = queue->end->prev;
	}
	queue->num--;

	if(pthread_mutex_unlock(lock) == -1){
		printf(strerror(errno));
		exit(1);
	}
	return b;
}

buffer_t* bqueue_pop_wait(bqueue_t* queue){
	buffer_t* b = NULL;
	pthread_mutex_t* lock = &(queue->lock);

	// Lock queue
	if(pthread_mutex_lock(lock) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_on_error();
	}
	if(queue->num == 0)
		logmsg(LOGLEVEL_DEBUG, "No buffers, unlocking and waiting for signal");
	while (queue->num == 0) {
		if(pthread_cond_wait(&(queue->condition), lock) == -1){
			logmsg(LOGLEVEL_ERROR, strerror(errno));
			exit_on_error();
		}
	}
	if(queue->num == 1){
		b = queue->start;
		queue->start = NULL;
		queue->end = NULL;
		queue->num--;
	} else {
		buffer_t* tmp = queue->end->prev;
		b = queue->end;
		tmp->next = NULL;
		queue->end = tmp;		
		queue->num--;
	}
	if(pthread_mutex_unlock(lock) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_on_error();
	}
	return b;
}

int bqueue_push_new(bqueue_t* queue){
	logmsg(LOGLEVEL_DEBUG, "\t Allocating new buffer");
	buffer_t* buf = (buffer_t*) malloc(sizeof(buffer_t));

	if(buf ==NULL)
		return ENOMEM;

	memset(buf, 0, sizeof(buffer_t));
	buf->data = malloc(queue->itemsize*queue->buffersize);
	buf->index = 0;

	if(buf->data == NULL)
		return ENOMEM;

	if(queue->num == 0){
		queue->end = buf;
		queue->start = buf;
	} else {
		queue->start->prev = buf;
		buf->next = queue->start;
		buf->prev = 0;
		queue->start = buf;
	}
	queue->num++;
	return 0;
}
