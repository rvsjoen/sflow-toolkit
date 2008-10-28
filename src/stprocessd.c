#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

#include "logger.h"
#include "util.h"

#define MSG_QUEUE_NAME "/sflow"

int main(int argc, char** argv)
{
	struct mq_attr attr;
	memset(&attr, 0, sizeof(struct mq_attr));

	mqd_t queue;
	queue = mq_open (MSG_QUEUE_NAME, O_RDONLY);
	if(queue == -1)
	{
		printf("%s\n", strerror(errno));
		exit(1);
	}

	char buf[1024];
	while(true)
	{
		mq_getattr(queue, &attr);
		printf ("%u messages are currently on the queue.\n", attr.mq_curmsgs);

		int res = mq_receive(queue, buf, 8192, NULL);
		if(res == -1){
			printf("%s\n", strerror(errno));
			exit(1);
		}
		printf("Got message: %s\n", buf);
	}
	mq_close (queue);
	return EXIT_SUCCESS;
}
