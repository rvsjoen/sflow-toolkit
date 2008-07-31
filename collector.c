#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "logger.h"
#include "sflow_parser.h"
#include "filesorter.h"

#define DEFAULT_FLUSH_INTERVAL 100
#define DEFAULT_PORT 6343
#define RECEIVE_BUFFER_SIZE 1500

#define MAX_DATAGRAM_SAMPLES 10
#define NUM_BUFFERS 10

int port 			= DEFAULT_PORT;
int flush_interval 	= DEFAULT_FLUSH_INTERVAL;
int print_interval	= 1000;
char* interface 	= NULL;
char* cwd			= NULL;

int cnt 			= 0;
int cnt_total_f  	= 0;
int cnt_total_c 	= 0;
int cnt_current_f 	= 0;
int cnt_current_c	= 0;

int sock_fd 		= 0;
int time_start 		= 0;
int time_end 		= 0;
bool compress		= false;
bool exit_var		= 0;


pthread_mutex_t locks[NUM_BUFFERS];
pthread_t write_thread;

/*-----------------------------------------------------------------------------
 *  How many packets to capture before we close and go home
 *  This is a command line option
 *-----------------------------------------------------------------------------*/
unsigned int num_packets	= -1;

/*-----------------------------------------------------------------------------
 * This is declared in the logger but we need to change it according to the
 * command line options
 *-----------------------------------------------------------------------------*/
extern int log_level;

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
int buffer_current_collect 	= 0;
int buffer_current_flush 	= 0;
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
void printInHex(unsigned char* pkt, int len){
        printf("\n\tHEX dump\n\t");
	int j=0;
        int i;
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
void printSingleLineHex(unsigned char* pkt, int len){
	int i;
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
int createAndBindSocket(){
	int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(sock_fd == -1){
		log(LOGLEVEL_ERROR, strerror(errno));
		exit_collector(1); // Critical error
	}
	log(LOGLEVEL_DEBUG, "Created new socket (%u)", sock_fd);

	// Convert the port to a string before passing it to getaddrinfo
	char portbuf[5];
	sprintf(portbuf, "%u", port);
	struct addrinfo* adr = 0;

	getaddrinfo(interface, portbuf, NULL, &adr);

	log(LOGLEVEL_DEBUG, "Binding socket to interface");

	if(bind(sock_fd, adr->ai_addr, sizeof(struct sockaddr)) == -1){
		log(LOGLEVEL_ERROR, strerror(errno));
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
	int i = 0;
	for(;i<NUM_BUFFERS;i++)
	{
		log(LOGLEVEL_DEBUG, "De-allocating memory for buffer %u", i);
		free(sfbuf[i]);
		free(scbuf[i]);
	}
}

int write_f = 0;
int write_c = 0;

void* writeBufferToDisk(void*){
	while(exit_var != true){
	//	samples_f_write = sfbuf[buffer_current_flush];
	//	samples_c_write = scbuf[buffer_current_flush];

		log(LOGLEVEL_DEBUG, "Waiting for buffer %u to be ready to flush", buffer_current_flush);
		pthread_mutex_lock(&locks[buffer_current_flush]);
		log(LOGLEVEL_DEBUG, "Write buffer thread flushing buffer[%u]", buffer_current_flush);	
	
		log(LOGLEVEL_DEBUG, "Writing to disk (%u flow samples, %u counter samples", write_f, write_c);
	
		int i=0;
		for(;i<write_f;i++){
			SFFlowSample* fls = sfbuf[buffer_current_flush];
	//		SFFlowSample* fls = &samples_f_write[i];
			addSampleToFile(fls, cwd, SFTYPE_FLOW);
		}
		write_f = 0;

		i=0;
		for(;i<write_c;i++){
			SFCntrSample* fls = scbuf[buffer_current_flush];
	//		SFCntrSample* fls = &samples_c_write[i];
			addSampleToFile(fls, cwd, SFTYPE_CNTR);
		}
		write_c = 0;
	
		log(LOGLEVEL_DEBUG, "Done writing to disk, zeroing buffer");
		zeroAll(sfbuf[buffer_current_flush], scbuf[buffer_current_flush]);

		pthread_mutex_unlock(&locks[buffer_current_flush]);
		log(LOGLEVEL_DEBUG, "Flushing finished, buffer %u unlocked",buffer_current_flush);

		buffer_current_flush = (buffer_current_flush + 1 )%NUM_BUFFERS;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  flushLists
 *  Description:  Flushes the contents of the lists to disk
 * =====================================================================================
 */
void flushLists()
{
	log(LOGLEVEL_DEBUG, "Requesting flush to disk : %u flow samples, %u counter samples", cnt_current_f, cnt_current_c);

	int b = buffer_current_collect;
	// Take the next buffer and lock it for the collector
	buffer_current_collect = (buffer_current_collect+1)%NUM_BUFFERS;
	log(LOGLEVEL_DEBUG, "Locking buffer %u for collecting", buffer_current_collect);
	pthread_mutex_lock(&locks[buffer_current_collect]);

	write_f = cnt_current_f;
	write_c = cnt_current_c;

	pthread_mutex_unlock(&locks[b]);
	log(LOGLEVEL_DEBUG, "Unlocking buffer %u for flushing", b);

	// Change the pointes to the current buffer
	samples_f = sfbuf[buffer_current_collect];
	samples_c = scbuf[buffer_current_collect];

	cnt_current_f = 0;
	cnt_current_c = 0;

	log(LOGLEVEL_DEBUG, "Collecting to buffer %u", buffer_current_collect);
	
}

void collect()
{
	sock_fd = createAndBindSocket();
	log(LOGLEVEL_DEBUG, "Waiting for packets...");
	int i = 0;
	time_t t = 0;
	for(;i<num_packets;i++)
	{
		unsigned char buf[RECEIVE_BUFFER_SIZE];
		int bytes_received = recv(sock_fd, &buf, RECEIVE_BUFFER_SIZE, 0);
		cnt++;
		if(time_start == 0)time_start = time(NULL);
		if(t==0) t = time(NULL);
		parseDatagram(buf, bytes_received);
		if(print_hex) printInHex(buf, bytes_received);

		time_t d_t = time(NULL) - t;
		if(cnt%print_interval==0)
		{
			log(LOGLEVEL_INFO, "Processed %u packets", cnt);
		}

		if(cnt%flush_interval==0 && flush_interval>0)
		{
			log(LOGLEVEL_INFO, "%u seconds since last update, effective sampling rate: %.1f samples/sec", d_t, ((cnt_current_f+cnt_current_c)/(double)d_t));
			t = time(NULL);
			flushLists();
		}
		time_end = time(NULL);

	}
	log(LOGLEVEL_DEBUG, "Socket closed", cnt);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  hook_exit
 *  Description:  This function will cause the application to exit cleanly
 * =====================================================================================
 */
void hook_exit(){
	flushLists();

	exit_var = true;
	pthread_join(write_thread, NULL);

	log(LOGLEVEL_DEBUG, "Closing socket");
	close(sock_fd);

	time_t d_t = time_end - time_start;
	log(LOGLEVEL_INFO, "Ran for %u seconds", d_t);
	log(LOGLEVEL_INFO, "Total: %u packet(s), %u flow samples, %u counter samples, average sampling rate %.1f samples/sec", cnt, cnt_total_f, cnt_total_c, (cnt_total_f+cnt_total_c)/static_cast<double>(d_t));
	log(LOGLEVEL_DEBUG, "Releasing all resources");

	freeAll();
	log(LOGLEVEL_DEBUG, "Exiting");

	exit_collector(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handle_signal
 *  Description:  Signal handler for SIGINT (CTRL+C)
 * =====================================================================================
 */
void handle_signal(int sig){
	hook_exit();
}

void allocateMemory(){
	// We create the arrays to store the samples in and allocate the memory for them and zero it
	int i = 0;
	for(;i<NUM_BUFFERS;i++)
	{
		pthread_mutex_init(&locks[i], NULL);
		SFFlowSample* s_f = (SFFlowSample*) malloc(sizeof(SFFlowSample)*flush_interval*MAX_DATAGRAM_SAMPLES);
		SFCntrSample* s_c = (SFCntrSample*) malloc(sizeof(SFCntrSample)*flush_interval*(MAX_DATAGRAM_SAMPLES/4));
		log(LOGLEVEL_DEBUG, "Allocated memory for buffer %u", i);
		sfbuf[i] = s_f;
		scbuf[i] = s_c;
		zeroAll(s_f, s_c);
		log(LOGLEVEL_DEBUG, "Zeroed memory in buffer %u", i);
	}
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

	log(LOGLEVEL_DEBUG, "Parsed command line");
	log(LOGLEVEL_DEBUG, "Size of a single flow sample is %u bytes", sizeof(SFFlowSample));
	log(LOGLEVEL_DEBUG, "Size of a single cntr sample is %u bytes", sizeof(SFCntrSample));

	allocateMemory();
	pthread_mutex_lock(&locks[buffer_current_collect]);

samples_f = sfbuf[buffer_current_collect];
samples_c = scbuf[buffer_current_collect];
//samples_f_write = sfbuf[buffer_current_flush];
//samples_c_write = scbuf[buffer_current_flush];

	log(LOGLEVEL_DEBUG, "Initialized buffer pointers to buffer 0");
	
	// If current working directory was not set from the command line we set it here
	if(cwd==NULL)
		cwd = get_current_dir_name();
	log(LOGLEVEL_DEBUG, "Data directory : %s", cwd);

	// Start the thread which is going to help us write when the buffers are marked for flushing
	log(LOGLEVEL_DEBUG, "Starting diskwriter");
	pthread_create(&write_thread, NULL, writeBufferToDisk, NULL);

	// Start collecting 
	log(LOGLEVEL_DEBUG, "Starting collector");
	collect();

	hook_exit();
}
