#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <time.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <mqueue.h>

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "agentlist.h"
#include "bufferqueue.h"
#include "util.h"
#include "logger.h"
#include "sflowparser.h"
#include "samplestore.h"
#include "statistics.h"
#include "configparser.h"
#include "messaging.h"

#define RECEIVE_BUFFER_SIZE 	1500
#define MAX_DATAGRAM_SAMPLES 	10

extern stcollectd_config_t stcollectd_config;

buffer_t* buffer_cw_flow 	= NULL;	// Current write flow
buffer_t* buffer_cw_cntr 	= NULL; // Current write counter
buffer_t* buffer_cc_flow 	= NULL; // Current collect flow
buffer_t* buffer_cc_cntr 	= NULL; // Current collect counter

bqueue_t* buffers_free_cntr 	= NULL;
bqueue_t* buffers_free_flow 	= NULL;
bqueue_t* buffers_flush_cntr 	= NULL;
bqueue_t* buffers_flush_flow 	= NULL;

uint64_t total_datagrams		= 0;
uint64_t total_samples_flow		= 0;
uint64_t total_samples_cntr		= 0;
uint64_t total_bytes_written	= 0;

uint32_t sock_fd 		= 0;
uint32_t time_start 	= 0;
uint32_t time_end 		= 0;

extern bool daemonize;
bool exit_writer_thread			= false;
bool exit_collector_thread		= false;

pthread_t write_thread;
pthread_t collect_thread;
mqd_t queue;

/*-----------------------------------------------------------------------------
 * This is declared in the logger but we need to change it according to the
 * command line options
 *-----------------------------------------------------------------------------*/
extern uint32_t log_level;

/*-----------------------------------------------------------------------------
 *  This is declared in the sflow parser but we need to change it according
 *  to the command line options
 *-----------------------------------------------------------------------------*/
extern bool print_parse;

/*-----------------------------------------------------------------------------
 *  Defined whether or not we should print a hex dump of each packet
 *  This is a command line option
 *-----------------------------------------------------------------------------*/
bool print_hex;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  usage
 *  Description:  Print usage information
 * =====================================================================================
 */
void usage(){
	printf("Usage : stcollectd [options]\n");	
	printf("See manual page for details about available options\n");	
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  parseCommandLine
 *  Description:  Parse the options for command line and print usage information
 *  if we encounter unrecognized options
 * =====================================================================================
 */
void parseCommandLine(int argc, char** argv){
	int opt;
	while((opt = getopt(argc, argv, "vhp:i:n:xXo:f:zdb:P:s:c:"))  != -1)
	{
		switch(opt)
		{
			case 'x': print_parse 		= true; 			break;
			case 'X': print_hex 		= true; 			break;
			case 'd': daemonize 		= false;			break;
			case 'v': log_level++; 							break;
			case 'h': usage(); exit_collector(0); 			break;
			default : usage(); exit_collector(1);			break;
		}
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  createAndBindSocket
 *  Description:  If no port or interface is specified we use the default port and
 *  the first interface returned by getaddrinfo
 * =====================================================================================
 */
uint32_t createAndBindSocket(){
	int32_t sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock_fd == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_collector(1); // Critical error
	}
	logmsg(LOGLEVEL_DEBUG, "Created new socket (%u)", sock_fd);

	// Convert the port to a string before passing it to getaddrinfo
	char portbuf[5];
	sprintf(portbuf, "%u", stcollectd_config.port);
	struct addrinfo* adr = 0;

	getaddrinfo(stcollectd_config.interface, portbuf, NULL, &adr);

	logmsg(LOGLEVEL_DEBUG, "Binding socket to interface");

	if(bind(sock_fd, adr->ai_addr, sizeof(struct sockaddr)) == -1){
		logmsg(LOGLEVEL_ERROR, strerror(errno));
		exit_collector(1); // Critical error
	}

	freeaddrinfo(adr);
	return sock_fd;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  freeMemory
 *  Description:  Release the memory held by the buffer queues
 * =====================================================================================
 */
void freeMemory(){

	logmsg(LOGLEVEL_DEBUG, "Waiting for remaining buffers to empty");
	while(buffers_flush_flow->num>0 || buffers_flush_cntr->num>0 ){
		sleep(1);
	}

	// When we get here, the writing thread is dead and the write queue is empty
	// so we dont need any more buffers
///	bqueue_destroy(buffers_free_flow);
//	bqueue_destroy(buffers_free_cntr);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  flushLists
 *  Description:  Flushes the contents of the current buffers by passing
 *  			  them to the flush queue
 * =====================================================================================
 */
void flushLists(){

	logmsg(LOGLEVEL_DEBUG, "Collected %u flow samples and %u counter samples,  requesting flush", 
			buffer_cc_flow->count, buffer_cc_cntr->count);

	// Push the current buffers to the flush queue
	logmsg(LOGLEVEL_DEBUG, "Pushing buffers to write queue");
	bqueue_push(buffers_flush_flow, buffer_cc_flow);
	bqueue_push(buffers_flush_cntr, buffer_cc_cntr);

	// Pop new buffers from the free queue, allocate if not buffers are free
	logmsg(LOGLEVEL_DEBUG, "Popping new buffers");
	buffer_cc_flow = bqueue_pop(buffers_free_flow);
	buffer_cc_cntr = bqueue_pop(buffers_free_cntr);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writeBufferToDisk
 *  Description:  Main function for the writing thread
 * =====================================================================================
 */
void* writeBufferToDisk(){

	while(true){

		// This will wait until there is something to flush
		logmsg(LOGLEVEL_DEBUG, "Waiting for next buffers to flush");
		buffer_cw_flow = bqueue_pop_wait(buffers_flush_flow);
		buffer_cw_cntr = bqueue_pop_wait(buffers_flush_cntr);

		// We got something, write it to disk
		logmsg(LOGLEVEL_INFO, "Writing to disk (%u flow samples, %u counter samples) from buffer",
			buffer_cw_flow->count, buffer_cw_cntr->count);
		if( buffer_cw_flow->count >0 ){
			uint32_t i=0;
			SFFlowSample* fls = buffer_cw_flow->data;

			for(;i<buffer_cw_flow->count;i++){
				addSampleToFile(fls, stcollectd_config.datadir, SFTYPE_FLOW);
				fls++;
			}
			buffer_cw_flow->count = 0;
		}
		if( buffer_cw_cntr->count > 0 ){
			uint32_t i=0;
			SFCntrSample* cs = buffer_cw_cntr->data;
			for(;i<buffer_cw_cntr->count;i++){
				addSampleToFile(cs, stcollectd_config.datadir, SFTYPE_CNTR);
				cs++;
			}
			buffer_cw_cntr->count = 0;
		}

		logmsg(LOGLEVEL_DEBUG, "Done writing to disk");

		// Push these buffers back on the free queue and NULL'ify the flush buffer pointers
		bqueue_push(buffers_free_flow, buffer_cw_flow);
		bqueue_push(buffers_free_cntr, buffer_cw_cntr);
		buffer_cw_flow = NULL;
		buffer_cw_cntr = NULL;
	}
	logmsg(LOGLEVEL_DEBUG, "Writing thread exiting");
	void* p; p=NULL; return p;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  collect
 *  Description:  Main function for the collecting thread
 * =====================================================================================
 */
void* collect(){
	sock_fd = createAndBindSocket();

	time_t t 	= 0; // This is when we start the collecting thread
	time_start = time(NULL);

	stats_init_stcollectd();
	stats_update_stcollectd_realtime(time_start, 0, total_datagrams, total_samples_flow, total_samples_cntr, total_bytes_written);

	uint32_t flush_cnt = 0;
	struct sockaddr addr;
	socklen_t addr_len;

	logmsg(LOGLEVEL_DEBUG, "Waiting for packets...");
	while(!exit_collector_thread)
	{

		time_t time_current = time(NULL); // The time we entered the loop

		unsigned char buf[RECEIVE_BUFFER_SIZE];

		addr_len = sizeof(struct sockaddr);
		uint32_t bytes_received = recvfrom(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
		total_datagrams++;
		flush_cnt++;

		if(t == 0) t = time_current;

		parseDatagram(buf, bytes_received, (struct sockaddr_in*)&addr);
		if(print_hex) printInHex(buf, bytes_received);

		time_t d_t = time_current - t;

		// Print a message every print_interval packets received
		if(total_datagrams%stcollectd_config.print_interval==0)
			logmsg(LOGLEVEL_INFO, "Processed %u packets", total_datagrams);

		// We flush if the interval has expired or if one of the buffers are full
		if(((unsigned int)d_t >= stcollectd_config.flush_interval) || buffer_cc_flow->count >= (stcollectd_config.buffer_size - MAX_DATAGRAM_SAMPLES) || buffer_cc_cntr->count >= (stcollectd_config.buffer_size - MAX_DATAGRAM_SAMPLES)){
			float srate = 0;
			if(d_t != 0)
				srate = (buffer_cc_flow->count+buffer_cc_cntr->count)/(double)d_t;

			logmsg(LOGLEVEL_INFO, "%u seconds since last update, effective sampling rate: %.1f samples/sec", d_t, srate);
			uint32_t bytes = (buffer_cc_cntr->count*sizeof(SFCntrSample)+buffer_cc_flow->count*sizeof(SFFlowSample));
			total_bytes_written += bytes;
			stats_update_stcollectd(d_t, 0, total_datagrams, total_samples_flow, total_samples_cntr, total_bytes_written);
			flushLists();
			stats_update_stcollectd_realtime(time_start, 0, total_datagrams, total_samples_flow, total_samples_cntr, total_bytes_written);
			t = time_current;
		}
		time_end = time_current; // We stopped collecting here, used to calculate the total average sampling rate
	}
	logmsg(LOGLEVEL_DEBUG, "Collecting thread exiting");
	void* p; p=NULL; return p;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  hook_exit
 *  Description:  This function will cause the application to exit cleanly
 * =====================================================================================
 */
void hook_exit(int signal){
	// We know the collecting thread already finished, if not we would not be here
	// So we can do some collector-related cleanup
	logmsg(LOGLEVEL_DEBUG, "Performing cleanup routine");
	logmsg(LOGLEVEL_DEBUG, "Closing socket");
	close(sock_fd);

	// Wait for the writing thread
	exit_writer_thread = true;
	logmsg(LOGLEVEL_DEBUG, "Waiting for writing thread to finish");
	pthread_join(write_thread, NULL);

	time_t d_t = time_end - time_start;
	logmsg(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	logmsg(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", 
			total_datagrams, total_samples_flow, total_samples_cntr, (total_samples_flow+total_samples_cntr)/(double)(d_t));
	logmsg(LOGLEVEL_DEBUG, "Exiting with signal %u", signal);
	
	freeMemory();
	destroyLogger();
	close_msg_queue(queue);

	if(!daemonize)
		disable_echo(false);
	exit_collector(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handle_signal
 *  Description:  Signal handler for signals that do shutdown
 * =====================================================================================
 */
void handle_signal(int sig){
	logmsg(LOGLEVEL_INFO, "Shutdown initiated");
	exit_collector_thread = true;
	pthread_join(collect_thread, NULL);
	hook_exit(sig);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  allocateMemory
 *  Description:  Do the initial allocation of memory and buffer queues
 * =====================================================================================
 */
void allocateMemory(){
	// Initialize the buffer queues
	logmsg(LOGLEVEL_DEBUG, "Initializing buffer queue with %u buffers for each sample type", stcollectd_config.buffer_num);
	buffers_free_flow 	= bqueue_init(stcollectd_config.buffer_num, stcollectd_config.buffer_size, sizeof(SFFlowSample));
	buffers_free_cntr 	= bqueue_init(stcollectd_config.buffer_num, stcollectd_config.buffer_size,sizeof(SFCntrSample));

	logmsg(LOGLEVEL_DEBUG, "Initializing empty buffer queue");
	buffers_flush_flow 	= bqueue_init(0, stcollectd_config.buffer_size, sizeof(SFFlowSample));
	buffers_flush_cntr 	= bqueue_init(0, stcollectd_config.buffer_size, sizeof(SFCntrSample));

	// Pop the first buffers for the collecting thread
	buffer_cc_flow = bqueue_pop(buffers_free_flow);
	buffer_cc_cntr = bqueue_pop(buffers_free_cntr);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printConfig
 *  Description:  Print out some information about the current configuration
 *  			  of the collector
 * =====================================================================================
 */
void printConfig(){
	logmsg(LOGLEVEL_DEBUG, "Configuration:");
	logmsg(LOGLEVEL_DEBUG, "\tInterface: %s", stcollectd_config.interface);
	logmsg(LOGLEVEL_DEBUG, "\tPort: %u", stcollectd_config.port);
	logmsg(LOGLEVEL_DEBUG, "\tLoglevel: %u", stcollectd_config.loglevel);
	logmsg(LOGLEVEL_DEBUG, "\tHashing bits: %u", stcollectd_config.hashbits);
	logmsg(LOGLEVEL_DEBUG, "\tPrint interval: %u datagrams", stcollectd_config.print_interval);
	logmsg(LOGLEVEL_DEBUG, "\tFlush interval: %u seconds", stcollectd_config.flush_interval);
	logmsg(LOGLEVEL_DEBUG, "\tStats interval: %u seconds", stcollectd_config.stats_interval);
	logmsg(LOGLEVEL_DEBUG, "\tBuffer size: %u samples", stcollectd_config.buffer_size);
	logmsg(LOGLEVEL_DEBUG, "\tBuffer num: %u", stcollectd_config.buffer_num);
	logmsg(LOGLEVEL_DEBUG, "\tData dir: %s", stcollectd_config.datadir);
	logmsg(LOGLEVEL_DEBUG, "\tTemp dir: %s", stcollectd_config.tmpdir);
	logmsg(LOGLEVEL_DEBUG, "\tMessage queue: %s", stcollectd_config.msgqueue);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  The main entry point for the application
 * =====================================================================================
 */
int main(int argc, char** argv){

	parseCommandLine(argc, argv);

	if(daemonize)
		daemonize_me();
	else
		disable_echo(false);

	initLogger("stcollectd");
	agentlist_init();
	parse_config_file(NULL, argv[0]);
	printConfig();

	logmsg(LOGLEVEL_DEBUG, "Trapping signals...");
	(void)signal(SIGINT, 	handle_signal);
	(void)signal(SIGHUP, 	handle_signal);
	(void)signal(SIGQUIT, 	handle_signal);
	(void)signal(SIGABRT, 	handle_signal);
	(void)signal(SIGTERM, 	handle_signal);

	logmsg(LOGLEVEL_DEBUG, "Size of a single flow sample is %u bytes", sizeof(SFFlowSample));
	logmsg(LOGLEVEL_DEBUG, "Size of a single cntr sample is %u bytes", sizeof(SFCntrSample));

	// Allocate the buffer queues and initial buffers
	allocateMemory();

	logmsg(LOGLEVEL_DEBUG, "Initializing message queue: %s", stcollectd_config.msgqueue);
	queue = create_msg_queue(stcollectd_config.msgqueue);

	// Start collecting thread
	logmsg(LOGLEVEL_DEBUG, "Starting collector");
	pthread_create(&collect_thread, NULL, collect, NULL);

	// Start the thread which is going to help us write when the buffers are marked for flushing
	logmsg(LOGLEVEL_DEBUG, "Starting diskwriter");
	pthread_create(&write_thread, NULL, writeBufferToDisk, NULL);

	// Wait for the collecting thread to finish before cleaning up
	pthread_join(collect_thread, NULL);

	// Perform the exit routine, this includes waiting for threads
	handle_signal(0);
}
