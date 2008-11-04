#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>

#include "messaging.h"
#include "logger.h"
#include "util.h"
#include "dataparser.h"

extern uint32_t log_level;

void process_file(const msg_t* m){
	if(m->type == SFTYPE_FLOW){
		logmsg(LOGLEVEL_DEBUG, "Processing flow file (%s)", m->filename);
		process_file_flow(m->filename, m->agent);
	} else if (m->type == SFTYPE_CNTR) {
		logmsg(LOGLEVEL_DEBUG, "Processing counter file (%s)", m->filename);
	}

}

int main(int argc, char** argv)
{
	UNUSED_ARGUMENT(argc);
	UNUSED_ARGUMENT(argv);

	log_level = LOGLEVEL_DEBUG;
	mqd_t queue;
	queue = open_msg_queue(MSG_QUEUE_NAME);

	// This needs to loop indefinately
	msg_t m;
	memset(&m, 0, sizeof(msg_t));
	recv_msg(queue, &m);
	process_file(&m);

	close_msg_queue(queue);
	return EXIT_SUCCESS;
}
