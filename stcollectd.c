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

#define DEFAULT_FLUSH_INTERVAL 100
#define DEFAULT_PORT 6343
#define RECEIVE_BUFFER_SIZE 1500

#define MAX_DATAGRAM_SAMPLES 10
#define NUM_BUFFERS 10

uint32_t port 			= DEFAULT_PORT;
uint32_t flush_interval = DEFAULT_FLUSH_INTERVAL;
uint32_t print_interval	= 1000;

char* interface 	= NULL;
char* cwd			= NULL;
char** validagents	= NULL;
cmph_t *h 			= NULL;
agent_stat* agent_stats = NULL;

//TODO See if some of these can be removed
uint32_t cnt 			= 0;
uint32_t cnt_total_f  	= 0;
uint32_t cnt_total_c 	= 0;
uint32_t cnt_current_f 	= 0;
uint32_t cnt_current_c	= 0;
uint32_t sock_fd 		= 0;
uint32_t time_start 	= 0;
uint32_t time_end 		= 0;
uint32_t write_f 		= 0;
uint32_t write_c 		= 0;

bool compress		= false;
bool exit_var		= 0;

pthread_mutex_t locks[NUM_BUFFERS];
pthread_t write_thread;

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

SFFlowSample* sfbuf[NUM_BUFFERS];
SFCntrSample* scbuf[NUM_BUFFERS];
uint32_t buffer_current_collect = 0;
uint32_t buffer_current_flush 	= 0;

//TODO See if these can be removed
SFFlowSample* samples_f;
SFCntrSample* samples_c;

//SFFlowSample* samples_f_write;
//SFCntrSample* samples_c_write;

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
	printf("Usage : collector [-p <port>] [-i <listen address>] [-h] [-v[v]]] [-x] [-X] [-z] [-f <value>] [-n <value>] [-o <dir>]\n");	
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
	printf("\t-z\tCompress the saved files using bzip2\n\n");
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
	while((opt = getopt(argc, argv, "vhp:i:n:xXo:f:z"))  != -1)
	{
		switch(opt)
		{
			case 'p': port = atoi(optarg); 			break;
			case 'i': interface = optarg; 			break;
			case 'v': log_level++; 					break;
			case 'n': num_packets = atoi(optarg); 	break;
			case 'f': flush_interval = atoi(optarg);break;
			case 'x': print_parse = true; 			break;
			case 'X': print_hex = true; 			break;
			case 'h': usage(); help(); exit_collector(0); 	break;
			case 'o': cwd = optarg; 				break;
			case 'z': compress = true;				break;
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
	memset(sf, 1, sizeof(SFFlowSample)*flush_interval*MAX_DATAGRAM_SAMPLES);
	memset(sc, 1, sizeof(SFCntrSample)*flush_interval*(MAX_DATAGRAM_SAMPLES/4));
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  freeAll
 *  Description:  Release the memory occupied by the sample arrays
 * =====================================================================================
 */
void freeAll(){
	uint32_t i = 0;
	for(;i<NUM_BUFFERS;i++)
	{
		logmsg(LOGLEVEL_DEBUG, "De-allocating memory for buffer %u", i);
		free(sfbuf[i]);
		free(scbuf[i]);
	}
}


void* writeBufferToDisk(){

	while(exit_var != true){

		logmsg(LOGLEVEL_DEBUG, "Waiting for buffer %u to be ready to flush", buffer_current_flush);
		pthread_mutex_lock(&locks[buffer_current_flush]);
		logmsg(LOGLEVEL_DEBUG, "Write buffer thread flushing buffer[%u]", buffer_current_flush);	
		logmsg(LOGLEVEL_DEBUG, "Writing to disk (%u flow samples, %u counter samples", write_f, write_c);
	
		uint32_t i=0;
		for(;i<write_f;i++){
			SFFlowSample* fls = sfbuf[buffer_current_flush];
			addSampleToFile(fls, cwd, SFTYPE_FLOW);
		}
		write_f = 0;

		i=0;
		for(;i<write_c;i++){
			SFCntrSample* fls = scbuf[buffer_current_flush];
			addSampleToFile(fls, cwd, SFTYPE_CNTR);
		}

		write_c = 0;
	
		logmsg(LOGLEVEL_DEBUG, "Done writing to disk, zeroing buffer");
		zeroAll(sfbuf[buffer_current_flush], scbuf[buffer_current_flush]);

		pthread_mutex_unlock(&locks[buffer_current_flush]);
		logmsg(LOGLEVEL_DEBUG, "Flushing finished, buffer %u unlocked",buffer_current_flush);

		buffer_current_flush = (buffer_current_flush + 1 )%NUM_BUFFERS;
	}

	void* p; return p;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  flushLists
 *  Description:  Flushes the contents of the lists to disk
 * =====================================================================================
 */
void flushLists()
{
	logmsg(LOGLEVEL_DEBUG, "Requesting flush to disk : %u flow samples, %u counter samples", cnt_current_f, cnt_current_c);

	uint32_t b = buffer_current_collect;
	// Take the next buffer and lock it for the collector
	buffer_current_collect = (buffer_current_collect+1)%NUM_BUFFERS;
	logmsg(LOGLEVEL_DEBUG, "Locking buffer %u for collecting", buffer_current_collect);
	pthread_mutex_lock(&locks[buffer_current_collect]);

	write_f = cnt_current_f;
	write_c = cnt_current_c;

	pthread_mutex_unlock(&locks[b]);
	logmsg(LOGLEVEL_DEBUG, "Unlocking buffer %u for flushing", b);

	// Change the pointes to the current buffer
	samples_f = sfbuf[buffer_current_collect];
	samples_c = scbuf[buffer_current_collect];

	cnt_current_f = 0;
	cnt_current_c = 0;

	logmsg(LOGLEVEL_DEBUG, "Collecting to buffer %u", buffer_current_collect);
	
}

void collect()
{
	sock_fd = createAndBindSocket();
	logmsg(LOGLEVEL_DEBUG, "Waiting for packets...");
	uint32_t i = 0;
	time_t t = 0;
	for(;i<num_packets;i++)
	{
		unsigned char buf[RECEIVE_BUFFER_SIZE];
		uint32_t bytes_received = recv(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0);
		cnt++;
	
		//TODO REMOVE Do something useful here, this is just for testing
		//	const char *key = "10.144.46.50";
		//	unsigned int id = cmph_search(h, key, strlen(key));

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
			logmsg(LOGLEVEL_INFO, "%u seconds since last update, effective sampling rate: %.1f samples/sec", d_t, ((cnt_current_f+cnt_current_c)/(double)d_t));
			t = time(NULL);
			flushLists();
		}
		time_end = time(NULL);

	}
	logmsg(LOGLEVEL_DEBUG, "Socket closed", cnt);
}

void destroyHash(){
	cmph_destroy(h);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  hook_exit
 *  Description:  This function will cause the application to exit cleanly
 * =====================================================================================
 */
void hook_exit(int signal){
	flushLists();

	exit_var = true;
	pthread_join(write_thread, NULL);

	logmsg(LOGLEVEL_DEBUG, "Closing socket");
	close(sock_fd);

	time_t d_t = time_end - time_start;
	logmsg(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	logmsg(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", cnt, cnt_total_f, cnt_total_c, (cnt_total_f+cnt_total_c)/(double)(d_t));
	logmsg(LOGLEVEL_DEBUG, "Releasing all resources");

	freeAll();
	logmsg(LOGLEVEL_DEBUG, "Exiting with signal %u", signal);

	destroyHash();

	exit_collector(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handle_signal
 *  Description:  Signal handler for SIGINT (CTRL+C)
 * =====================================================================================
 */
void handle_signal(int sig){
	hook_exit(sig);
}

void allocateMemory(){
	// We create the arrays to store the samples in and allocate the memory for them and zero it
	int i = 0;
	for(;i<NUM_BUFFERS;i++)
	{
		pthread_mutex_init(&locks[i], NULL);
		SFFlowSample* s_f = (SFFlowSample*) malloc(sizeof(SFFlowSample)*flush_interval*MAX_DATAGRAM_SAMPLES);
		SFCntrSample* s_c = (SFCntrSample*) malloc(sizeof(SFCntrSample)*flush_interval*(MAX_DATAGRAM_SAMPLES/4));
		logmsg(LOGLEVEL_DEBUG, "Allocated memory for buffer %u", i);
		sfbuf[i] = s_f;
		scbuf[i] = s_c;
		zeroAll(s_f, s_c);
		logmsg(LOGLEVEL_DEBUG, "Zeroed memory in buffer %u", i);
	}
}


void initHash(){
	FILE * keys_fd = fopen("agents.txt", "r");
	logmsg(LOGLEVEL_DEBUG, "Reading agents from file");
	if (keys_fd == NULL) {
		fprintf(stderr, "File \"agents.txt\" not found\n");
		exit(1);
	}
	cmph_io_adapter_t *source = cmph_io_nlfile_adapter(keys_fd);

	cmph_config_t *config = cmph_config_new(source);
	cmph_config_set_algo(config, CMPH_CHM);
	h = cmph_new(config);
	cmph_config_destroy(config);

	//Destroy hash
	cmph_io_nlfile_adapter_destroy(source);

	rewind(keys_fd);

	int size = cmph_size(h);
	validagents=malloc(sizeof(char*) * size);
	int maxkeylength = 15;
	int i;
	for( i=0; i<size; i++ )
	{
		// Add 2 because of newline and null-terminator
		char tmp[maxkeylength+2];
		fgets(tmp, maxkeylength+2, keys_fd);

		// Now we create the char array to hold the key
		char*  thiskey;
		thiskey=malloc(sizeof(char)*(strlen(tmp)));
		memset(thiskey, 0, sizeof(char)*(strlen(tmp)));
		strncpy(thiskey, tmp, strlen(tmp)-1);
		thiskey[strlen(tmp)] = '\0';
		validagents[i] = thiskey;
	}
	fclose(keys_fd);

	// Allocate some space for the agent stats
	agent_stats = calloc(cmph_size(h), sizeof(agent_stat));
	memset(agent_stats, 0, sizeof(agent_stat)*cmph_size(h));
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  The main entry point for the application
 * =====================================================================================
 */
int main(int argc, char** argv){
	parseCommandLine(argc, argv);
	disable_echo(true);

	(void)signal(SIGINT, handle_signal);
	(void)signal(SIGHUP, handle_signal);
	(void)signal(SIGQUIT, handle_signal);
	(void)signal(SIGABRT, handle_signal);
	(void)signal(SIGTERM, handle_signal);

	logmsg(LOGLEVEL_DEBUG, "Parsed command line");
	logmsg(LOGLEVEL_DEBUG, "Size of a single flow sample is %u bytes", sizeof(SFFlowSample));
	logmsg(LOGLEVEL_DEBUG, "Size of a single cntr sample is %u bytes", sizeof(SFCntrSample));

	allocateMemory();
	pthread_mutex_lock(&locks[buffer_current_collect]);

	samples_f = sfbuf[buffer_current_collect];
	samples_c = scbuf[buffer_current_collect];

	logmsg(LOGLEVEL_DEBUG, "Initialized buffer pointers to buffer 0");
	
	// If current working directory was not set from the command line we set it here
	if(cwd==NULL)
		cwd = get_current_dir_name();
	logmsg(LOGLEVEL_DEBUG, "Data directory : %s", cwd);

	initHash();

	// Start the thread which is going to help us write when the buffers are marked for flushing
	logmsg(LOGLEVEL_DEBUG, "Starting diskwriter");
	pthread_create(&write_thread, NULL, writeBufferToDisk, NULL);

	// Start collecting 
	logmsg(LOGLEVEL_DEBUG, "Starting collector");
	collect();

	hook_exit(0);
}
