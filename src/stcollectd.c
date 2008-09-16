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

#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "logger.h"
#include "sflowparser.h"
#include "filesorter.h"

// Default configuration parameters
#define DEFAULT_FLUSH_INTERVAL 	100
#define DEFAULT_PORT 			6343
#define RECEIVE_BUFFER_SIZE 	1500
#define MAX_DATAGRAM_SAMPLES 	10
#define DEFAULT_CONFIG_FILE		"/etc/stcollectd.conf"
#define DEFAULT_AGENTS_FILE		"/etc/stcollectd.agents"
#define NUM_BUFFERS 			10
#define PRINT_INTERVAL			1000


uint32_t print_interval	= PRINT_INTERVAL;
uint32_t num_buffers	= NUM_BUFFERS;
uint32_t port 			= DEFAULT_PORT;
uint32_t flush_interval = DEFAULT_FLUSH_INTERVAL;

char* interface 	= NULL;
char* cwd			= NULL;
char* file_agents	= DEFAULT_AGENTS_FILE;
char* file_config 	= DEFAULT_CONFIG_FILE;

char** validagents		= NULL;
int num_agents 			= 0;
cmph_t *h 				= NULL;
agent_stat* agent_stats = NULL;

uint32_t cnt 			= 0;
uint32_t cnt_total_f  	= 0;
uint32_t cnt_total_c 	= 0;
uint32_t sock_fd 		= 0;
uint32_t time_start 	= 0;
uint32_t time_end 		= 0;

bool exit_writer_thread			= false;
bool exit_collector_thread		= false;
bool daemonize					= true;

pthread_mutex_t* locks;
pthread_t write_thread;
pthread_t collect_thread;

/*-----------------------------------------------------------------------------
 *  How many packets to capture before we close and go home
 *  This is a command line option
 *-----------------------------------------------------------------------------*/
uint32_t num_packets = -1;

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

/*-----------------------------------------------------------------------------
 *  Our collection to hold pointers to the parsed samples from the parser
 *-----------------------------------------------------------------------------*/

SFFlowSample** sfbuf;
SFCntrSample** scbuf;

uint32_t* scnum;
uint32_t* sfnum;

uint32_t buffer_current_collect = 0;
uint32_t buffer_current_flush 	= 0;

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
void usage()
{
	printf("Usage : stcollectd [options]\n");	
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  help
 *  Description:  Print help information
 * =====================================================================================
 */
void help()
{
	printf("\n");
	printf("\t-f<val>\tSet the flushing interval in packets, the collector will flush data to disk when <val> packets are captured\n\n");
	printf("\t-h\tShow this help\n\n");
	printf("\t-i<val>\tAddress to listen on, if no address is specified the collector will choose the first it finds\n\n");
	printf("\t-n<val>\tDefine the number of packets to capture before exiting, if this is not specified it will capture until told otherwise\n\n");
	printf("\t-o<val>\tDefine the directory to save data in, in this directory the collector will create a data folder, NO TRAILING SLASH!\n\n");
	printf("\t-p<val>\tPort to listen on, if no port is specified the default (6343) will be used\n\n");
	printf("\t-v\tPrint out more information\n\n");
	printf("\t-vv\tEven more information\n\n");
	printf("\t-x\tPrints the parsed information\n\n");
	printf("\t-X\tPrints the HEX dump of each recieved packet\n\n");
	printf("\t-d\tDo not daemonize\n\n");
	printf("\t-c <filename>\tUse another configuration file (default is /etc/stcollectd.conf)\n\n");
	printf("\t-a <filename>\tUse another agent file (default is /etc/stcollectd.agents\n\n");
	printf("\t-b <val>\tSet the number of buffers\n\n");
	printf("\t-P <val>\tSet the interval for printing statistics (in packets)\n\n");
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
	while((opt = getopt(argc, argv, "vhp:i:n:xXo:f:zc:a:db:P:"))  != -1)
	{
		switch(opt)
		{
			case 'p': port = atoi(optarg); 					break;
			case 'i': interface = optarg; 					break;
			case 'v': log_level++; 							break;
			case 'n': num_packets = atoi(optarg); 			break;
			case 'f': flush_interval = atoi(optarg);		break;
			case 'x': print_parse = true; 					break;
			case 'X': print_hex = true; 					break;
			case 'h': usage(); help(); exit_collector(0); 	break;
			case 'o': cwd = optarg; 						break;
			case 'a': file_agents = optarg;					break;
			case 'c': file_config = optarg;					break;
			case 'd': daemonize = false;					break;
			case 'b': num_buffers = atoi(optarg); 			break;
			case 'P': print_interval = atoi(optarg); 		break;
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
 *         Name:  zeroAll
 *  Description:  Zero the memory occupied by the sample arrays
 * =====================================================================================
 */
void zeroAll(SFFlowSample* sf, SFCntrSample* sc){
	memset(sf, 0, sizeof(SFFlowSample)*flush_interval*MAX_DATAGRAM_SAMPLES);
	memset(sc, 0, sizeof(SFCntrSample)*flush_interval*(MAX_DATAGRAM_SAMPLES/4));
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  freeAll
 *  Description:  Release the memory occupied by the sample arrays
 * =====================================================================================
 */
void freeAll(){

	uint32_t i = 0;
	for(;i<num_buffers;i++)
	{
		logmsg(LOGLEVEL_DEBUG, "De-allocating memory for buffer %u", i);
		free(sfbuf[i]);
		free(scbuf[i]);
	}
	free(scbuf);
	free(sfbuf);
	free(sfnum);
	free(scnum);
}

void destroyHash(){
	cmph_destroy(h);
}

void printAgentStats(){
	int i = 0;
	logmsg(LOGLEVEL_DEBUG, "Agent Stats:");
	for( ; i<num_agents; i++ )
	{
		agent_stat* as = &agent_stats[i];
		logmsg(LOGLEVEL_DEBUG, "agent %s received %u ", validagents[as->agent_index], as->tot_datagrams_received);
	}
}

void exit_on_error() {
	if(!daemonize)
		disable_echo(false);
	logmsg(LOGLEVEL_DEBUG, "Exiting on error");
	exit(1);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  flushLists
 *  Description:  Flushes the contents of the lists to disk
 * =====================================================================================
 */
void flushLists()
{
	logmsg(LOGLEVEL_DEBUG, "Collected %u flow samples and %u counter samples to buffer %u, requesting flush", 
			sfnum[buffer_current_collect], scnum[buffer_current_collect], buffer_current_collect);

	// Take the next buffer and lock it for the collector, if it is still locked my the writing process we wait and lose samples
	uint32_t buffer_next_collect = (buffer_current_collect+1)%num_buffers;

	// Lock the next buffer we are going to collect into
	logmsg(LOGLEVEL_DEBUG, "Locking buffer %u for collecting", buffer_next_collect);
	if (pthread_mutex_lock(&locks[buffer_next_collect]) == 0)
		logmsg(LOGLEVEL_DEBUG, "Buffer %u successfully locked for collecting", buffer_next_collect);

	// Unlock the previous buffer we collected to so that the writing thread can write it to disk
	logmsg(LOGLEVEL_DEBUG, "Unlocking buffer %u for flushing", buffer_current_collect);
	if(pthread_mutex_unlock(&locks[buffer_current_collect])==0)
		logmsg(LOGLEVEL_DEBUG, "Buffer %u successfully unlocked for flushing", buffer_current_collect);

	buffer_current_collect = buffer_next_collect;
	logmsg(LOGLEVEL_DEBUG, "Collecting to buffer %u", buffer_next_collect);
}

void* writeBufferToDisk(){

	bool time_to_shutdown = false;
	while(true){

		if(exit_writer_thread && !time_to_shutdown){
			logmsg(LOGLEVEL_DEBUG, "Starting to shut down writing thread");
			time_to_shutdown = true;
		}

		if(exit_writer_thread && time_to_shutdown){
			logmsg(LOGLEVEL_DEBUG, "Final flush of buffer %u", buffer_current_flush);
			if(pthread_mutex_trylock(&locks[buffer_current_flush])==0)
				break;
		} else {

			logmsg(LOGLEVEL_DEBUG, "Waiting for buffer %u to be ready to flush", buffer_current_flush);
			if (pthread_mutex_lock(&locks[buffer_current_flush])==0)
				logmsg(LOGLEVEL_DEBUG, "Write buffer thread flushing buffer %u ", buffer_current_flush);

		}

		logmsg(LOGLEVEL_DEBUG, "Writing to disk (%u flow samples, %u counter samples) from buffer %u", 
				sfnum[buffer_current_flush], scnum[buffer_current_flush], buffer_current_flush);

		if( sfnum[buffer_current_flush] >0 ){
			uint32_t i=0;

			SFFlowSample* fls = sfbuf[buffer_current_flush];

			for(;i<sfnum[buffer_current_flush];i++){
				addSampleToFile(fls, cwd, SFTYPE_FLOW);
				fls++;
			}
			sfnum[buffer_current_flush] = 0;
		}
		if(scnum[buffer_current_flush] > 0){
			uint32_t i=0;
			SFCntrSample* cs = scbuf[buffer_current_flush];
			for(;i<scnum[buffer_current_flush];i++){
				addSampleToFile(cs, cwd, SFTYPE_CNTR);
				cs++;
			}
			scnum[buffer_current_flush] = 0;
		}

		logmsg(LOGLEVEL_DEBUG, "Done writing to disk, zeroing buffer %u", buffer_current_flush);
		zeroAll(sfbuf[buffer_current_flush], scbuf[buffer_current_flush]);

		if (pthread_mutex_unlock(&locks[buffer_current_flush]) == 0)
			logmsg(LOGLEVEL_DEBUG, "Flushing finished, buffer %u unlocked",buffer_current_flush);
		buffer_current_flush = (buffer_current_flush + 1 )%num_buffers;
	}
	logmsg(LOGLEVEL_DEBUG, "Writing thread exiting");
	void* p; p=NULL; return p;
}

void* collect()
{
	sock_fd = createAndBindSocket();
	logmsg(LOGLEVEL_DEBUG, "Waiting for packets...");
	uint32_t i = 0;
	time_t t = 0;
	for(;i<num_packets && !exit_collector_thread;i++)
	{
		unsigned char buf[RECEIVE_BUFFER_SIZE];
		uint32_t bytes_received = recv(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0);
		cnt++;
	
		if(time_start == 0)time_start = time(NULL);
		if(t==0) t = time(NULL);
		parseDatagram(buf, bytes_received);
		if(print_hex) printInHex(buf, bytes_received);

		time_t d_t = time(NULL) - t;
		if(cnt%print_interval==0)
		{
			logmsg(LOGLEVEL_INFO, "Processed %u packets", cnt);
		}
		if(cnt%flush_interval==0 && flush_interval>0)
		{
			logmsg(LOGLEVEL_INFO, "%u seconds since last update, effective sampling rate: %.1f samples/sec", d_t, 
					((scnum[buffer_current_collect]+sfnum[buffer_current_collect])/(double)d_t));
			t = time(NULL);
			flushLists();
		}
		time_end = time(NULL);
	}
	// Lock the next one (empty) to prevent over-running
	pthread_mutex_lock(&locks[(buffer_current_collect+1)%num_buffers]);
	logmsg(LOGLEVEL_DEBUG, "Collecting finished, unlocking the last buffer");
	pthread_mutex_unlock(&locks[buffer_current_collect]);
	logmsg(LOGLEVEL_DEBUG, "Collecting thread exiting");
	exit_writer_thread = true;
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
	logmsg(LOGLEVEL_DEBUG, "Waiting for writing thread to finish");
	pthread_join(write_thread, NULL);

	time_t d_t = time_end - time_start;
	logmsg(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	logmsg(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", 
			cnt, cnt_total_f, cnt_total_c, (cnt_total_f+cnt_total_c)/(double)(d_t));

	printAgentStats();
	logmsg(LOGLEVEL_DEBUG, "Releasing all resources");
	freeAll();
	logmsg(LOGLEVEL_DEBUG, "Exiting with signal %u", signal);
	
	destroyHash();
	destroyLogger();
	if(!daemonize)
		disable_echo(false);

	exit_collector(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handle_signal
 *  Description:  Signal handler for SIGINT (CTRL+C)
 * =====================================================================================
 */
void handle_signal(int sig){
	logmsg(LOGLEVEL_DEBUG, "Shutdown initiated");
	exit_collector_thread = true;
	pthread_join(collect_thread, NULL);
	hook_exit(sig);
}

void allocateMemory(){
	locks = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*num_buffers);
	logmsg(LOGLEVEL_DEBUG, "Allocating memory for %u buffer pointers", num_buffers);
	sfbuf = (SFFlowSample**) malloc(sizeof(SFFlowSample*)*num_buffers);
	scbuf = (SFCntrSample**) malloc(sizeof(SFCntrSample*)*num_buffers);

	// We create the arrays to store the samples in and allocate the memory for them and zero it
	uint32_t i = 0;
	for(;i<num_buffers;i++)
	{
		pthread_mutex_init(&locks[i], NULL);
		SFFlowSample* s_f = (SFFlowSample*) malloc(sizeof(SFFlowSample)*flush_interval*MAX_DATAGRAM_SAMPLES);
		SFCntrSample* s_c = (SFCntrSample*) malloc(sizeof(SFCntrSample)*flush_interval*(MAX_DATAGRAM_SAMPLES/4));

		if(s_f == NULL || s_c == NULL){
			logmsg(LOGLEVEL_ERROR, "Error allocating memory");
			exit_on_error();
		} else {

			logmsg(LOGLEVEL_DEBUG, "Allocated memory for buffer %u", i);
			sfbuf[i] = s_f;
			scbuf[i] = s_c;
			zeroAll(s_f, s_c);
			logmsg(LOGLEVEL_DEBUG, "Zeroed memory in buffer %u", i);
		}
	}

	logmsg(LOGLEVEL_DEBUG, "Allocating memory for %u counters", num_buffers);
	scnum = (uint32_t*) malloc(sizeof(uint32_t)*num_buffers);
	sfnum = (uint32_t*) malloc(sizeof(uint32_t)*num_buffers);
	memset(scnum, 0, sizeof(uint32_t)*num_buffers);
	memset(sfnum, 0, sizeof(uint32_t)*num_buffers);
}

void initHash(){
	FILE * keys_fd = fopen(file_agents, "r");
	logmsg(LOGLEVEL_DEBUG, "Reading agents from file %s", file_agents);

	if (keys_fd == NULL) {
		logmsg(LOGLEVEL_ERROR, "File \"%s\" not found", file_agents);
		exit_on_error();
	}
	cmph_io_adapter_t *source = cmph_io_nlfile_adapter(keys_fd);

	cmph_config_t *config = cmph_config_new(source);
	cmph_config_set_algo(config, CMPH_CHM);
	h = cmph_new(config);
	cmph_config_destroy(config);

	//Destroy hash
	cmph_io_nlfile_adapter_destroy(source);

	rewind(keys_fd);

	num_agents = cmph_size(h);
	validagents=malloc(sizeof(char*) * num_agents);
	int maxkeylength = 15;
	int i;
	for( i=0; i<num_agents; i++ )
	{
		// Add 2 because of newline and null-terminator
		char tmp[maxkeylength+2];
		fgets(tmp, maxkeylength+2, keys_fd);

		// Now we create the char array to hold the key
		char*  thiskey;

		thiskey=malloc(sizeof(char)*(strlen(tmp)));
		memset(thiskey, 0, sizeof(char)*(strlen(tmp)));

		strncpy(thiskey, tmp, strlen(tmp)-1);
//		thiskey[strlen(tmp)] = '\0';

		validagents[i] = thiskey;
	}
	fclose(keys_fd);

	// Allocate some space for the agent stats
	agent_stats = calloc(num_agents, sizeof(agent_stat));
	memset(agent_stats, 0, sizeof(agent_stat)*num_agents);
}

void daemonize_me() {	
		pid_t pid, sid;

		if ( getppid() == 1 ) return;

		// Fork off the parent process
		pid = fork();
		if (pid < 0) 
			exit(EXIT_FAILURE);
	
		// If we got a good PID, then we can exit the parent process
		if (pid > 0) 
			exit(EXIT_SUCCESS);
	
		// Change the file mode mask
		//umask(0);
	
		// Create a new SID for the child process
		sid = setsid();
		if (sid < 0)
			exit(EXIT_FAILURE);
	
		// Change the current working directory
		if ((chdir("/")) < 0) 
			exit(EXIT_FAILURE);
	
		// Close out the standard file descriptors
		freopen( "/dev/null", "r", stdin);
		freopen( "/dev/null", "w", stdout);
		freopen( "/dev/null", "w", stderr);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  The main entry point for the application
 * =====================================================================================
 */
int main(int argc, char** argv){

	initLogger();
	parseCommandLine(argc, argv);

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

	allocateMemory();
	pthread_mutex_lock(&locks[buffer_current_collect]);

	logmsg(LOGLEVEL_DEBUG, "Initialized buffer pointers to buffer %u", buffer_current_collect);
	
	// If current working directory was not set from the command line we set it here
	if(cwd==NULL){
		logmsg(LOGLEVEL_ERROR, "Data directory not set");
		exit_on_error();
	}

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

	// Start the thread which is going to help us write when the buffers are marked for flushing
	logmsg(LOGLEVEL_DEBUG, "Starting diskwriter");
	pthread_create(&write_thread, NULL, writeBufferToDisk, NULL);

	// Start collecting thread
	logmsg(LOGLEVEL_DEBUG, "Starting collector");
	pthread_create(&collect_thread, NULL, collect, NULL);

	// Wait for the collecting thread to finish before cleaning up
	pthread_join(collect_thread, NULL);

	// Perform the exit routine, this includes waiting for threads
	handle_signal(0);
}
