#ifndef __messaging_h__
#define __messaging_h__

#define MSG_QUEUE_NAME "/sflow"
#define MSG_SIZE		512

#include <stdlib.h>
#include <stdint.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include "logger.h"
#include "sflowparser.h"

typedef struct _msg {
	uint32_t agent;
	SFSample_t type;
	char filename[256];
} msg_t;

mqd_t create_msg_queue(char* queue);
mqd_t open_msg_queue(char* queue);
void destroy_msg_queue(mqd_t q, char* qname);
void close_msg_queue(mqd_t queue);
void send_msg(mqd_t q, msg_t* m);
void recv_msg(mqd_t q, msg_t* m);

#endif


