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

uint64_t total_datagrams		= 0;
uint64_t total_samples_flow		= 0;
uint64_t total_samples_cntr		= 0;
uint64_t total_bytes_written	= 0;

uint32_t sock_fd 		= 0;
uint32_t time_start 	= 0;
uint32_t time_end 		= 0;

bool daemonize				= true;
bool exit_collector_thread	= false;

pthread_t write_thread;
pthread_t collect_thread;
mqd_t queue;

/*-----------------------------------------------------------------------------
 * This is declared in the logger but we need to change it according to the
 * command line options
 *-----------------------------------------------------------------------------*/
extern uint32_t log_level;

/*-----------------------------------------------------------------------------
 *  If true, no files are written, packets are collected, buffered and dumped
 *-----------------------------------------------------------------------------*/
bool debug_nowrite = false;

/*-----------------------------------------------------------------------------
 *  This is used in the sflow parser but we need to change it according
 *  to the command line options
 *-----------------------------------------------------------------------------*/
bool debug_print = false;

/*-----------------------------------------------------------------------------
 *  Defined whether or not we should print a hex dump of each packet
 *  This is a command line option
 *-----------------------------------------------------------------------------*/
bool debug_hex = false;

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
	while((opt = getopt(argc, argv, "nxXdvh"))  != -1)
	{
		switch(opt)
		{
			case 'n': debug_nowrite		= true;		break;
			case 'x': debug_print		= true;		break;
			case 'X': debug_hex			= true;		break;
			case 'd': daemonize 		= false;	break;
			case 'v': log_level++; 					break;
			case 'h': usage(); exit_collector(0); 	break;
			default : usage(); exit_collector(1);	break;
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
 *         Name:  collect
 *  Description:  Main function for the collecting thread
 * =====================================================================================
 */
void collect(){
	sock_fd = createAndBindSocket();

	time_t t 	= 0; // This is when we start the collecting thread
	time_start = time(NULL);

	stats_init_stcollectd();
	stats_update_stcollectd_realtime(time_start, 0, total_datagrams, total_samples_flow, total_samples_cntr, total_bytes_written);

	struct sockaddr addr;
	socklen_t addr_len;

	logmsg(LOGLEVEL_DEBUG, "Waiting for packets...");
	while(!exit_collector_thread)
	{
		time_t time_current = time(NULL); // The time we entered the loop

		// A buffer to hold our packet
		unsigned char buf[RECEIVE_BUFFER_SIZE];

		addr_len = sizeof(struct sockaddr);
		uint32_t bytes_received = recvfrom(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
		total_datagrams++;

		if(t == 0) t = time_current;

		parseDatagram(buf, bytes_received, (struct sockaddr_in*)&addr);
		if(debug_hex) printInHex(buf, bytes_received);

		// Print a message every print_interval packets received
		if(total_datagrams%stcollectd_config.print_interval==0)
			logmsg(LOGLEVEL_DEBUG, "Processed %u packets", total_datagrams);

		time_end = time_current; // We stopped collecting here, used to calculate the total average sampling rate
	}
	logmsg(LOGLEVEL_DEBUG, "Collecting thread exiting");
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

	time_t d_t = time_end - time_start;
	logmsg(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	logmsg(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", 
			total_datagrams, total_samples_flow, total_samples_cntr, (total_samples_flow+total_samples_cntr)/(double)(d_t));
	logmsg(LOGLEVEL_DEBUG, "Exiting with signal %u", signal);
	
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
	hook_exit(sig);
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

	initLogger(argv[0]);
	agentlist_init();
	parse_config_file(NULL, argv[0]);
	printConfig();

	logmsg(LOGLEVEL_DEBUG, "Trapping signals...");
	(void)signal(SIGINT, 	handle_signal);
	(void)signal(SIGHUP, 	handle_signal);
	(void)signal(SIGQUIT, 	handle_signal);
	(void)signal(SIGABRT, 	handle_signal);
	(void)signal(SIGTERM, 	handle_signal);

	logmsg(LOGLEVEL_DEBUG, "Initializing message queue: %s", stcollectd_config.msgqueue);
	queue = create_msg_queue(stcollectd_config.msgqueue);

	logmsg(LOGLEVEL_DEBUG, "Starting collector");
	collect();

	// Perform the exit routine, this includes waiting for threads
	handle_signal(0);
}
