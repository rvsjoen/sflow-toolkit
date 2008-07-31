#ifndef __timetree_h__
#define __timetree_h__

#include <stdint.h>
#include <time.h>
#include "sflow_parser.h"

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

void writePcapHeader(const char* filename);
void writePcapRecord(const char* filename, SFFlowSample* sample);
void writeToPcap(const char* filename, SFFlowSample* sample);
void writeToBinary(const char* filename, const void* data, SFSample_t type);
void addSampleToFile(const void* sample, char* root, SFSample_t type);
void getFilePath(uint32_t agent_address, time_t timestamp, char* filename);
void createFolder(char* s);

#endif
