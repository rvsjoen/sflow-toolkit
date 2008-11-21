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

	mqd_t q = mq_open(queue, O_CREAT|O_WRONLY, DEFFILEMODE, &attr);
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

	// Wait for 100 nano-seconds and then just move on (in case stprocessd dies, remove when sure this shit works)
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec= 100;

	memset(msg, 0, MSG_SIZE*sizeof(char));
	sprintf(msg, "%u %u %s %i", m->agent, m->timestamp, m->filename, m->type);
	logmsg(LOGLEVEL_DEBUG, "Sending message: %s", msg);

	if(mq_timedsend(q, msg, strlen(msg), 0, &t) == -1)
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
}

void recv_msg(mqd_t q, msg_t* m){
	char msg[MSG_SIZE];
	int len;
	len = mq_receive(q, msg, MSG_SIZE, 0);

	if(len == -1){
		logmsg(LOGLEVEL_ERROR, "msgqueue: %s", strerror(errno));
	} else {

		// Make sure there is a null terminator here
		msg[len] = '\0';
		logmsg(LOGLEVEL_DEBUG, "Received message: %s", msg);

		char filename[256];
		uint32_t agent;
		uint32_t timestamp;
		uint32_t type;
		sscanf(msg, "%u %u %s %u", &agent, &timestamp, filename, &type);
		m->agent = agent;
		m->type = type;
		m->timestamp = timestamp;
		strncpy(m->filename, filename, 256);
	}
}

uint32_t msg_pending(mqd_t q){
	struct mq_attr attr;
	memset(&attr, 0, sizeof(struct mq_attr));
	mq_getattr(q, &attr);
	return attr.mq_curmsgs;
}
