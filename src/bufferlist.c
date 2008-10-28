#include "bufferlist.h"

bufferlist_t* bufferlist_init(uint32_t num, uint32_t buffersize, uint32_t itemsize){
	logmsg(LOGLEVEL_DEBUG, "Allocating %u buffers with %u items of size %u",
			num,buffersize,itemsize);
	bufferlist_t* list = (bufferlist_t*) malloc(sizeof(bufferlist_t));
	memset(list, 0, sizeof(bufferlist_t));
	list->buffersize = buffersize;
	list->itemsize = itemsize;

	uint32_t i;
	for( i=0; i<num; i++ )
	{
		logmsg(LOGLEVEL_DEBUG, "\tAllocating memory for buffer %u", i);
		bufferlist_addbuffer(list, list->data);
	}
	return list;
}

void bufferlist_destroy(bufferlist_t* list){
	buffer_t* buf = list->data;
	uint32_t i;
	int n = list->total;
	for( i=0; i<n; i++ ){
		buffer_t* tmp = buf;
		buf = buf->next;
		bufferlist_removebuffer(list, tmp);
	}
	free(list);
}

buffer_t* bufferlist_addbuffer(bufferlist_t* list, buffer_t* b){
	
	buffer_t* buf = (buffer_t*) malloc(sizeof(buffer_t));
	memset(buf, 0, sizeof(buffer_t));
	buf->data = malloc(list->itemsize*list->buffersize);
	buf->index = list->total;

	char str[64];
	sprintf(str, "Buffer %u", list->total);
	strncpy(buf->data, str, strlen(str));

	char* foo = (char*) buf->data;
	foo[strlen(str)] = '\0';

	pthread_mutex_init(&(buf->lock),NULL);
	if(b == NULL){
		buf->next = buf;
		buf->prev = buf;
		list->data = buf;
	} else {
		buf->prev = b;
		buf->next = b->next;
		b->next->prev = buf;
		b->next = buf;
	}
	buf->list = list;
	list->total++;
	return buf;
}

void bufferlist_removebuffer(bufferlist_t* list, buffer_t* b){
	list->total--;
	free(b->data);
	b->prev->next = b->next;
	b->next->prev = b->prev;
	free(b);
}

int bufferlist_ratio(bufferlist_t* list){
	return (int)((float)list->used/list->total);
}

void print_buffers(bufferlist_t* list){
	printf("Printing list of %u buffers\n", list->total);
	buffer_t* b = list->data;
	uint32_t i;
	for( i = 0; i<list->total; i++ )
	{
		printf("%s\n", b->data);
		b = b->next;
	}
	printf("\n");
}

buffer_t* buffer_checkout(bufferlist_t* list, buffer_t* b)
{
	int res = pthread_mutex_trylock(&(b->next->lock));
	if(res == EBUSY){
			logmsg(LOGLEVEL_DEBUG, "Next buffer still busy, allocating new");
			buffer_t* newbuf = bufferlist_addbuffer(list, b->prev);
			pthread_mutex_lock(&(newbuf->lock));
			return newbuf;
	}
	return b->next;
}

buffer_t* buffer_checkout_wait(bufferlist_t* list, buffer_t* b)
{
	pthread_mutex_lock(&(b->lock));
	return b;
}

void buffer_checkin(bufferlist_t* list, buffer_t* b)
{
	pthread_mutex_unlock(&(b->lock));
}
