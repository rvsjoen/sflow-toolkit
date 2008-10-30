#include "bufferqueue.h"
bqueue_t* bqueue_init(uint32_t num, uint32_t buffersize, uint32_t itemsize){

	logmsg(LOGLEVEL_DEBUG, "Initializing buffer queue");
	bqueue_t* b = (bqueue_t*) malloc(sizeof(bqueue_t));
	memset(b, 0, sizeof(bqueue_t));

	pthread_mutex_init(&(b->lock), NULL);
	pthread_mutex_init(&(b->condition_mutex), NULL);
	
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
	free(b);
}

void bqueue_push(bqueue_t* queue, buffer_t* b){
	pthread_mutex_lock(&(queue->lock));
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
	pthread_mutex_unlock(&(queue->lock));
	pthread_cond_signal(&(queue->condition));
}

buffer_t* bqueue_pop(bqueue_t* queue){
	return bqueue_pop_wait(queue);
/*	buffer_t* b = NULL;
	pthread_mutex_lock(&(queue->lock));

	if(queue->num == 0)
		bqueue_push_new(queue);

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
	pthread_mutex_unlock(&(queue->lock));
	return b;*/
}

buffer_t* bqueue_pop_wait(bqueue_t* queue){
	buffer_t* b = NULL;
	pthread_mutex_lock(&(queue->lock));

	if(queue->num == 0) {
		pthread_mutex_unlock(&(queue->lock));
		pthread_cond_wait(&(queue->condition), &(queue->condition_mutex));
		pthread_mutex_lock(&(queue->lock));
	}

	if(queue->num <= 1){
		b = queue->start;
		queue->start = NULL;
		queue->end = NULL;
	} else {
		buffer_t* tmp = queue->end->prev;
		b = queue->end;
		tmp->next = NULL;
		queue->end = tmp;		
	}
	queue->num--;
	pthread_mutex_unlock(&(queue->lock));
	return b;
}

void bqueue_push_new(bqueue_t* queue){
	buffer_t* buf = (buffer_t*) malloc(sizeof(buffer_t));
	memset(buf, 0, sizeof(buffer_t));
	buf->data = malloc(queue->itemsize*queue->buffersize);
	buf->index = 0;

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
}
