#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <cmph.h>
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
#include "filesorter.h"
#include "statistics.h"
#include "configparser.h"

// Default configuration parameters
#define DEFAULT_FLUSH_INTERVAL 	30
#define DEFAULT_BUFFER_SIZE		1000
#define DEFAULT_PORT 			6343
#define RECEIVE_BUFFER_SIZE 	1500
#define MAX_DATAGRAM_SAMPLES 	10
#define DEFAULT_CONFIG_FILE		"/etc/stcollectd.conf"
#define NUM_BUFFERS 			10
#define PRINT_INTERVAL			1000
#define MSG_QUEUE_NAME 			"/sflow"

// configurable options
uint32_t print_interval	= PRINT_INTERVAL;
uint32_t num_buffers	= NUM_BUFFERS;
uint32_t port 			= DEFAULT_PORT;
uint32_t flush_interval = DEFAULT_FLUSH_INTERVAL;
uint32_t buffer_size	= DEFAULT_BUFFER_SIZE;

char* interface 		= NULL;
char* cwd				= NULL;
char* file_config 		= DEFAULT_CONFIG_FILE;

buffer_t* buffer_cw_flow 	= NULL;	// Current write flow
buffer_t* buffer_cw_cntr 	= NULL; // Current write counter
buffer_t* buffer_cc_flow 	= NULL; // Current collect flow
buffer_t* buffer_cc_cntr 	= NULL; // Current collect counter

bqueue_t* buffers_free_cntr 	= NULL;
bqueue_t* buffers_free_flow 	= NULL;
bqueue_t* buffers_flush_cntr 	= NULL;
bqueue_t* buffers_flush_flow 	= NULL;

agentlist_t* agents				= NULL;
char** validagents				= NULL;
cmph_t *h 						= NULL;
uint32_t num_agents 			= 0;

uint32_t cnt 			= 0;
uint32_t cnt_total_f  	= 0;
uint32_t cnt_total_c 	= 0;
uint32_t bytes_total	= 0;

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
 *         Name:  printInHex
 *  Description:  Print an unsigned char array using hex
 * =====================================================================================
 */
void printInHex(unsigned char* pkt, uint32_t len){
        printf("\n\tHEX dump\n\t");
	uint32_t j=0;
        uint32_t i;
        for(i=0; i<len; i++){
		if(j++%2 == 0)
			printf(" ");

                printf("%.2X", *pkt);
                pkt++;
		j++;
		if((i+1)%30 == 0)
			printf("\n\t");
        }
        printf("\n\n");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printSingleLineHex
 *  Description:  Used to print the array of unsigned chars on a single line in hex
 * =====================================================================================
 */
void printSingleLineHex(unsigned char* pkt, uint32_t len){
	uint32_t i;
	for(i=0; i<len; i++){
		printf("%.2X", *pkt);
		pkt++;
	}
}

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
			case 'c': file_config 		= optarg; 			break;
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
	sprintf(portbuf, "%u", port);
	struct addrinfo* adr = 0;

	getaddrinfo(interface, portbuf, NULL, &adr);

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
 *         Name:  destroyHash
 *  Description:  Free the memory allocated to the hash and the agent list
 * =====================================================================================
 */
void destroyHash(){
	cmph_destroy(h);
	agentlist_destroy(agents);
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

	// When we get here, the writing thread is dead
	// We dont need any more new buffers
	bqueue_destroy(buffers_free_flow);
	bqueue_destroy(buffers_free_cntr);
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

	bool time_to_shutdown = false;

	
	while(true){
		if(exit_writer_thread && !time_to_shutdown){
			logmsg(LOGLEVEL_DEBUG, "Starting to shut down writing thread");
			time_to_shutdown = true;
		}
		
		if(exit_writer_thread && time_to_shutdown){
//			logmsg(LOGLEVEL_DEBUG, "Final flush of buffer %u", buffer_current_flush);
	//		if(pthread_mutex_trylock(&locks[buffer_current_flush])==0)
	//			break;
		} else {
//			logmsg(LOGLEVEL_DEBUG, "Waiting for buffer %u to be ready to flush", buffer_current_flush);
//			if (pthread_mutex_lock(&locks[buffer_current_flush])==0)
//				logmsg(LOGLEVEL_DEBUG, "Write buffer thread flushing buffer %u ", buffer_current_flush);

		}

		/*
		if(exit_writer_thread && time_to_shutdown){
			logmsg(LOGLEVEL_DEBUG, "Final flush of buffers flow[%u] and cntr[%u]", buffer_cw_flow->index, buffer_cw_cntr->index);
			if(pthread_mutex_trylock(&(buffer_cw_flow->lock)) == 0 && pthread_mutex_trylock(&(buffer_cw_cntr->lock)) == 0)
				break;
		}*/

		// This will wait until there is something to flush
		logmsg(LOGLEVEL_DEBUG, "Waiting for next buffers to flush");
		buffer_cw_flow = bqueue_pop_wait(buffers_flush_flow);
		buffer_cw_cntr = bqueue_pop_wait(buffers_flush_cntr);

		// We got something, write it to disk
		logmsg(LOGLEVEL_DEBUG, "Writing to disk (%u flow samples, %u counter samples) from buffer",
			buffer_cw_flow->count, buffer_cw_cntr->count);
		if( buffer_cw_flow->count >0 ){
			uint32_t i=0;
			SFFlowSample* fls = buffer_cw_flow->data;

			for(;i<buffer_cw_flow->count;i++){
				addSampleToFile(fls, cwd, SFTYPE_FLOW);
				fls++;
			}
			buffer_cw_flow->count = 0;
		}
		if( buffer_cw_cntr->count > 0 ){
			uint32_t i=0;
			SFCntrSample* cs = buffer_cw_cntr->data;
			for(;i<buffer_cw_cntr->count;i++){
				addSampleToFile(cs, cwd, SFTYPE_CNTR);
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

		//We flushed a buffer, tell someone (like stprocessd)
		//msgQueue();
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
	logmsg(LOGLEVEL_DEBUG, "Waiting for packets...");
	init_stats();
	time_t t 	= 0; // This is when we start the collecting thread
	time_start = time(NULL);
	update_realtime_stats();

	uint32_t flush_cnt = 0;

	while(!exit_collector_thread)
	{

		time_t time_current = time(NULL); // The time we entered the loop

		unsigned char buf[RECEIVE_BUFFER_SIZE];
		uint32_t bytes_received = recv(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0);
		cnt++;
		flush_cnt++;

		if(t == 0) t = time_current;

		parseDatagram(buf, bytes_received);
		if(print_hex) printInHex(buf, bytes_received);

		time_t d_t = time_current - t;

		// Print a message every print_interval packets received
		if(cnt%print_interval==0)
			logmsg(LOGLEVEL_INFO, "Processed %u packets", cnt);

		// We flush if the interval has expired or if one of the buffers are full
		if(((unsigned int)d_t >= flush_interval) || buffer_cc_flow->count >= (buffer_size-MAX_DATAGRAM_SAMPLES) || buffer_cc_cntr->count >= (buffer_size - MAX_DATAGRAM_SAMPLES)){
			float srate = 0;
			if(d_t != 0)
				srate = (buffer_cc_flow->count+buffer_cc_cntr->count)/(double)d_t;

			logmsg(LOGLEVEL_INFO, "%u seconds since last update, effective sampling rate: %.1f samples/sec", d_t, srate);

			uint32_t bytes = (buffer_cc_cntr->count*sizeof(SFCntrSample)+buffer_cc_flow->count*sizeof(SFFlowSample));
			bytes_total += bytes;
			update_stats((int)srate, d_t, bytes);
			flushLists();
			update_realtime_stats();
			t = time_current;
		}
		time_end = time_current; // We stopped collecting here, used to calculate the total average sampling rate
	}
	logmsg(LOGLEVEL_DEBUG, "Collecting thread exiting");
	exit_writer_thread = true;
	void* p; p=NULL; return p;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  destroyQueue
 *  Description:  Close the message queue handle, do not unlink !
 * =====================================================================================
 */
void destroyQueue(){
	mq_close(queue);
//	mq_unlink(MSG_QUEUE_NAME);
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
	logmsg(LOGLEVEL_DEBUG, "Waiting for writing thread to finish");
	pthread_join(write_thread, NULL);

	time_t d_t = time_end - time_start;
	logmsg(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	logmsg(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", 
			cnt, cnt_total_f, cnt_total_c, (cnt_total_f+cnt_total_c)/(double)(d_t));

	agentlist_print_stats(agents);

	logmsg(LOGLEVEL_DEBUG, "Exiting with signal %u", signal);
	
	freeMemory();
	destroyHash();
	destroyQueue();
	destroyLogger();

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
	logmsg(LOGLEVEL_DEBUG, "Shutdown initiated");
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
	buffers_free_flow 	= bqueue_init(num_buffers, buffer_size, sizeof(SFFlowSample));
	buffers_free_cntr 	= bqueue_init(num_buffers, buffer_size, sizeof(SFCntrSample));
	buffers_flush_flow 	= bqueue_init(0, buffer_size, sizeof(SFFlowSample));
	buffers_flush_cntr 	= bqueue_init(0, buffer_size, sizeof(SFCntrSample));

	// Pop the first buffers for the collecting thread
	buffer_cc_flow = bqueue_pop(buffers_free_flow);
	buffer_cc_cntr = bqueue_pop(buffers_free_cntr);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initHash
 *  Description:  Initialize the agent hash and agent list
 * =====================================================================================
 */
void initHash(){
	if(validagents != NULL){
		cmph_io_adapter_t *source = cmph_io_vector_adapter(validagents, num_agents);
		cmph_config_t *config = cmph_config_new(source);
		cmph_config_set_algo(config, CMPH_CHM);
		h = cmph_new(config);
		cmph_config_destroy(config);
	
		//Destroy hash
		cmph_io_vector_adapter_destroy(source);

		agents = agentlist_init(num_agents);
		uint32_t i;
		for(i=0;i<num_agents;i++){
			agent_t* a = agent_get(agents, i);
			a->index = i;
			strncpy(a->address, validagents[i], strlen(validagents[i]));
		}
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printConfig
 *  Description:  Print out some information about the current configuration
 * =====================================================================================
 */
void printConfig(){
	logmsg(LOGLEVEL_DEBUG, "Flush Interval: %u seconds", flush_interval);
	logmsg(LOGLEVEL_DEBUG, "Print Interval: %u seconds", print_interval);
	logmsg(LOGLEVEL_DEBUG, "Buffer size: %u samples", buffer_size);
	logmsg(LOGLEVEL_DEBUG, "Initial buffers: %u", num_buffers);
	logmsg(LOGLEVEL_DEBUG, "Interface: %s:%u", interface, port);
	logmsg(LOGLEVEL_DEBUG, "Data directory: %s", cwd);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initQueue
 *  Description:  Initialize the messaging queue
 * =====================================================================================
 */
void initQueue(){
	queue = mq_open(MSG_QUEUE_NAME, O_CREAT|O_WRONLY, DEFFILEMODE, NULL);
	if(queue == -1){
		logmsg(LOGLEVEL_ERROR, "initQueue: %s", strerror(errno));
		exit_on_error();
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  msgQueue
 *  Description:  Send a message
 * =====================================================================================
 */
void msgQueue(){
	char buf[] = "This is a message!";
	mq_send(queue, buf, strlen(buf), 0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  The main entry point for the application
 * =====================================================================================
 */
int main(int argc, char** argv){

	initLogger();
	parseConfigFile(DEFAULT_CONFIG_FILE);
	parseCommandLine(argc, argv);
	if(strcmp(file_config, DEFAULT_CONFIG_FILE) != 0)
		parseConfigFile(file_config);

	printConfig();

	if(daemonize){
		daemonize_me();
	} else {
		disable_echo(true);
	}

	(void)signal(SIGINT, 	handle_signal);
	(void)signal(SIGHUP, 	handle_signal);
	(void)signal(SIGQUIT, 	handle_signal);
	(void)signal(SIGABRT, 	handle_signal);
	(void)signal(SIGTERM, 	handle_signal);

	logmsg(LOGLEVEL_DEBUG, "Parsed command line");
	logmsg(LOGLEVEL_DEBUG, "Size of a single flow sample is %u bytes", sizeof(SFFlowSample));
	logmsg(LOGLEVEL_DEBUG, "Size of a single cntr sample is %u bytes", sizeof(SFCntrSample));

	// Allocate the buffer queues and initial buffers
	allocateMemory();

	// If current working directory was not set from the command line we set it here
	// TODO This can probably be improved some
	if(cwd==NULL){
		logmsg(LOGLEVEL_ERROR, "Data directory not set");
		exit_on_error();
	}

	// Do this in a separate scope because it is cleaner with the tmp variables
	{
		// Check that the data folder exists
		char* tmp;
		tmp = get_current_dir_name();
		if(chdir(cwd) == -1) {
			logmsg(LOGLEVEL_ERROR, "Data directory: %s", strerror(errno));
			exit_on_error();
		}
		chdir(tmp);
	}
	logmsg(LOGLEVEL_DEBUG, "Data directory : %s", cwd);

	logmsg(LOGLEVEL_DEBUG, "Initializing agents hash");
	initHash();

	logmsg(LOGLEVEL_DEBUG, "Initializing message queue");
	initQueue();

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
