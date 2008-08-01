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

#include "sflow_parser.h"
#include "util.h"
#include "logger.h"

#define PCAP_MAGIC 0xa1b2c3d4;
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4

// The file header for a pcap file
typedef struct pcap_hdr_s {
	uint32_t magic_number;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t thiszone;
	uint32_t sigfigs;
	uint32_t snaplen;
	uint32_t network;
} pcap_hdr_t;

// Header for a single pcap record
typedef struct pcaprec_hdr_s {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t incl_len;
	uint32_t orig_len;
} pcaprec_hdr_t;

bool fileExists(const char* filename);
void writePcapHeader(const char* filename);
void writePcapRecord(const char* filename, SFFlowSample* sample);
void writeToPcap(const char* filename, SFFlowSample* sample);
void writeToBinary(const char* filename, const void* data, SFSample_t type);
void addSampleToFile(const void* sample, char* root, SFSample_t type);
void getFilePath(uint32_t agent_address, time_t timestamp, char* filename);
void createFolder(char* s);

#endif
