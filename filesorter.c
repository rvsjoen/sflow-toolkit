#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bzlib.h>

#include "sflow_parser.h"
#include "util.h"
#include "logger.h"
#include "filesorter.h"

#define PCAP_MAGIC 0xa1b2c3d4;
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4

void createFolder(char* s)
{
	char path[strlen(s)+1];
	strncpy(path, s, strlen(s));
	path[strlen(s)] = '\0';
	char* token = NULL;
	chdir("/");
	token = strtok(path, "/");
	do {
		if(chdir(token) == -1)
		{
			if(mkdir(token, (mode_t) 0777) == -1)
			{
				exit_collector(1);
			} else  {
				chdir(token);
			}
		}
	} while ( token = strtok(NULL, "/"));
}

bool fileExists(const char* filename)
{
	FILE* f;
	f = fopen(filename, "r");
	if(f == NULL){
		return false;
	} else {
		fclose(f);
		return true;
	}
}

void writePcapHeader(const char* filename)
{
	FILE* f;
	if((f=fopen(filename, "a")) == NULL){
		printf("%s\n", strerror(errno));
		exit(1);
	}
	pcap_hdr_t hdr;
	memset(&hdr, 0, sizeof(pcap_hdr_t));
	hdr.magic_number 	= PCAP_MAGIC;
	hdr.version_major	= PCAP_VERSION_MAJOR;
	hdr.version_minor	= PCAP_VERSION_MINOR;
	hdr.thiszone		= 0;
	hdr.sigfigs			= 0;
	hdr.snaplen			= 128;
	hdr.network			= 1; //Ethernet
	fwrite(&hdr, 1, sizeof(pcap_hdr_t), f);
	fclose(f);
}

void writePcapRecord(const char* filename, SFFlowSample* sample)
{
	FILE* f;
	if((f=fopen(filename, "a")) == NULL){
		printf("%s\n", strerror(errno));
		exit(1);
	}
	pcaprec_hdr_t rec;
	rec.ts_sec = time(NULL);
	rec.ts_usec = 0;
	rec.orig_len = sample->raw_header_frame_length;
	rec.incl_len = sample->raw_header_length;
	fwrite(&rec, 1, sizeof(pcaprec_hdr_t), f);
	fwrite(sample->raw_header, 1, sample->raw_header_length, f);
	fclose(f);
}

void writeToPcap(const char* filename, SFFlowSample* sample){
	if(!fileExists(filename))
		writePcapHeader(filename);
	 writePcapRecord(filename, sample);
}

void writeToBinary(const char* filename, const void* data, SFSample_t type){
	FILE* f;
	if((f=fopen(filename, "a")) == NULL)
	{
		log(LOGLEVEL_ERROR, "%s", strerror(errno));
		exit_collector(1);
	}
	if(type == SFTYPE_FLOW)
		fwrite(data, sizeof(SFFlowSample), 1, f);
	else if(type == SFTYPE_CNTR)
		fwrite(data, sizeof(SFCntrSample), 1, f);
	fclose(f);
}

void getFilePath(uint32_t agent_address, time_t timestamp, char* filename){
	sprintf(
			filename+strlen(filename),
			"%i.%i.%i.%i/",
			((agent_address & 0xff000000) >> 24), 
			((agent_address & 0x00ff0000) >> 16), 
			((agent_address & 0x0000ff00) >> 8), 
			(agent_address & 0x000000ff)
	);
	struct tm* t;
	t = localtime(&timestamp);
	strftime(filename+strlen(filename), 16, "%Y%m%d/%H/%M/", t);
}

void addSampleToFile(const void* sample, char* root, SFSample_t type)
{
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/", root);
	if(type == SFTYPE_FLOW){
		SFFlowSample* s = (SFFlowSample*) sample;
		getFilePath(s->agent_address, s->timestamp, filename);
		createFolder(filename);
		sprintf(filename+strlen(filename), "samples_flow.dat");
	}
	else if (type == SFTYPE_CNTR) {
		SFCntrSample* s = (SFCntrSample*) sample;
		getFilePath(s->agent_address, s->timestamp, filename);
		createFolder(filename);
		sprintf(filename+strlen(filename), "samples_cntr.dat");
	}
	writeToBinary(filename, sample, type);
}
