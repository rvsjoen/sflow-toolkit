#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <netinet/in.h>                                                                                   

#include <errno.h>
#include <string.h>

#include "logger.h"
#include "filesorter.h"
#include "sflow_parser.h"

bool write_pcap;

char* fin;
char* fout;

void usage()
{
	printf("Usage : sftconvert [-p] <filename>\n");
}

void help()
{
    printf("\n");
    printf("\t-p\tDump this file to pcap format\n\n");
}

void parseCommandLine(int argc, char** argv)
{
	int opt;
	while((opt=getopt(argc, argv, "phi:o:")) != -1)
	{
		switch(opt)
		{
			case 'p': write_pcap = true; 		break;
			case 'h': usage(); help(); exit(0); break;
			case 'i': fin = optarg; break;
			case 'o': fout = optarg; break;
			default : usage(); exit(1); 		break;
		}
	}
}

void init(){
	write_pcap = false;
}

int main(int argc, char** argv)
{
	init();
	parseCommandLine(argc, argv);
	printf("IN: %s\n", fin);
	printf("OUT: %s\n", fout);

	if(write_pcap && fin != NULL && fout != NULL){
		printf("Dumping to pcap\n");
		writePcapHeader(fout);

		FILE* f;
		if((f=fopen(fin, "r")) == NULL){
			log(LOGLEVEL_ERROR, "%s", strerror(errno));
			exit(1);
		}
		SFFlowSample sample;
		while(fread(&sample, sizeof(SFFlowSample), 1, f))
		{
			writePcapRecord(fout, &sample);
			memset(&sample, 0, sizeof(SFFlowSample));
		}
		fclose(f);
	} else {
		usage();
		exit(1);
	}
	return 0;
}

