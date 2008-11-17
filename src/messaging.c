#include "messaging.h"

mqd_t create_msg_queue(char* queue){

	struct rlimit lim;
	memset(&lim, 0, sizeof(struct rlimit));

	lim.rlim_cur = MSG_MAXMSGS * sizeof(struct msg_t*) + sizeof(msg_t) * MSG_MAXMSGS;
	lim.rlim_max = MSG_MAXMSGS * sizeof(struct msg_t*) + sizeof(msg_t) * MSG_MAXMSGS;
	setrlimit(RLIMIT_MSGQUEUE, &lim);

	getrlimit(RLIMIT_MSGQUEUE, &lim);
	logmsg(LOGLEVEL_DEBUG, "msgqueue: softlimit(%u) hardlimit(%u)", lim.rlim_cur, lim.rlim_max);

	struct mq_attr attr;
	memset(&attr, 0, sizeof(struct mq_attr));
	attr.mq_maxmsg = MSG_MAXMSGS;
	attr.mq_msgsize = sizeof(msg_t);

	mqd_t q = mq_open(queue, O_CREAT|O_WRONLY, 0700, &attr);
	if(q == -1)
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
	return q;
}

mqd_t open_msg_queue(char* queue){
	mqd_t q = mq_open(queue, O_RDONLY);
	if(q == -1)
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
	return q;
}

void close_msg_queue(mqd_t q){
	mq_close(q);
}

void destroy_msg_queue(mqd_t q, char* qname){
	close_msg_queue(q);
	mq_unlink(qname);
}

void send_msg(mqd_t q, msg_t* m){
	char msg[MSG_SIZE];

	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec= 1000;

	sprintf(msg, "%u %s %u", m->agent, m->filename, m->type);
	logmsg(LOGLEVEL_DEBUG, "Sending message: %s", msg);
	if(mq_timedsend(q, msg, strlen(msg), 0, &t) == -1)
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
}

void recv_msg(mqd_t q, msg_t* m){
	char msg[8192];
	if(mq_receive(q, msg, 8192, 0) == -1){
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
	} else {
 //		char msg[] = "2307580894 /home/sjoen/work/git/sftoolkit/src/samples_flow.dat 0";
		char filename[256];
		uint32_t agent;
		SFSample_t type;
		sscanf(msg, "%u %s %u", &agent, filename, &type);
//		printf("MSG: %s Agent: %u Filename: %s Type: %u\n", msg, agent, filename, type);
		m->agent = agent;
		m->type = type;
		strncpy(m->filename, filename, 256);
	}
}
