#ifndef __filesorter_h__
#define __filesorter_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <netinet/in.h>

#include "sflowparser.h"
#include "logger.h"
#include "util.h"

/*-----------------------------------------------------------------------------
 *  These are values written in the pcap header structure
 *-----------------------------------------------------------------------------*/
#define PCAP_MAGIC 0xa1b2c3d4;
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4

/*-----------------------------------------------------------------------------
 *  File header for writing to a pcap file
 *-----------------------------------------------------------------------------*/
typedef struct pcap_hdr_s {
	uint32_t magic_number;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t thiszone;
	uint32_t sigfigs;
	uint32_t snaplen;
	uint32_t network;
} pcap_hdr_t;

/*-----------------------------------------------------------------------------
 *  Header for a single pcap record
 *-----------------------------------------------------------------------------*/
typedef struct pcaprec_hdr_s {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t incl_len;
	uint32_t orig_len;
} pcaprec_hdr_t;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  fileExists
 *  Description:  Check if a file exists
 * =====================================================================================
 */
bool fileExists(const char* filename);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writePcapHeader
 *  Description:  
 * =====================================================================================
 */
void writePcapHeader(const char* filename);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writePcapRecord
 *  Description:  Write a sFlow SFFlowSample structure as a record in a pcap file
 * =====================================================================================
 */
void writePcapRecord(const char* filename, SFFlowSample* sample);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writeToPcap
 *  Description:  Writes a flow sample to a pcap file, if the file doesnt exist
 *  			  the file is created and a pcap header is written first
 * =====================================================================================
 */
void writeToPcap(const char* filename, SFFlowSample* sample);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  writeToBinary
 *  Description:  Write a sFlow sample structure of either flow or counter type
 *  			  into a binary file
 * =====================================================================================
 */
void writeToBinary(const char* filename, const void* data, SFSample_t type);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  addSampleToFile
 *  Description:  Determine the type of sample and call writeToBinary to write it
 *  			  into a file
 * =====================================================================================
 */
void addSampleToFile(const void* sample, char* root, SFSample_t type);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getFilePath
 *  Description:  This function builds the file path from the agent address, timestamp
 *  			  and requested filename
 * =====================================================================================
 */
void getFilePath(uint32_t agent_address, time_t timestamp, char* filename);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  createFolder
 *  Description:  Create a directory tree recursively
 * =====================================================================================
 */
void createFolder(char* s);

#endif
