#include "filesorter.h"
#include "agentlist.h"

extern agentlist_t* agents;
extern mqd_t queue;

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

void writeToPcap(const char* filename, SFFlowSample* sample)
{
	if(!fileExists(filename))
		writePcapHeader(filename);
	 writePcapRecord(filename, sample);
}

void getFilePath(uint32_t agent_address, time_t timestamp, char* filename)
{
	struct tm* t;
	t = localtime(&timestamp);
	strftime(filename+strlen(filename), 16, "%Y%m%d_%H%M_", t);
	sprintf(filename+strlen(filename), "%u_", agent_address);
}

void addSampleToFile(const void* sample, char* root, SFSample_t type)
{

	UNUSED_ARGUMENT(root);

	if(type == SFTYPE_FLOW){
		SFFlowSample* s = (SFFlowSample*) sample;
		
		char key[16];
		sprintf(key, "%i.%i.%i.%i",
				((s->agent_address & 0xff000000) >> 24),
				((s->agent_address & 0x00ff0000) >> 16),
				((s->agent_address & 0x0000ff00) >> 8),
				(s->agent_address & 0x000000ff)
			   );
		uint32_t id = cmph_search(h, key, strlen(key));
		agent_t* a = agent_get(agents, id); 

		if((uint32_t)s->timestamp/60 != a->fd_min_flow){
			logmsg(LOGLEVEL_DEBUG, "Updating flow file descriptor for %s", key);
			a->fd_min_flow = s->timestamp/60;

			// Generate the path to the next file
			char filename[256];
			memset(filename, 0, 256);
			getFilePath(s->agent_address, s->timestamp, filename);

			// filename is the name of next data segment
			sprintf(filename+strlen(filename), "flow");

			// If we had a previous file, close it and pass it on to the processing daemon
			if(a->fd_flow != 0){
				msg_t m;
				memset(&m, 0, sizeof(msg_t));
				m.agent = a->agent;
				m.type = SFTYPE_FLOW;
				m.timestamp = ((uint32_t)s->timestamp/60)-1;
				strncpy(m.filename, a->fn_flow, 256);
				send_msg(queue, &m);
			}

			int f;
			if ( (f=shm_open(filename, O_CREAT | O_RDWR, S_IRWXU)) == -1){
				logmsg(LOGLEVEL_ERROR, "%s", strerror(errno));
			}

			a->fd_flow = f;
			strncpy(a->fn_flow, filename, 256);
			a->agent = s->agent_address;
		}
		write(a->fd_flow, sample, sizeof(SFFlowSample));

	} else if(type == SFTYPE_CNTR){

		SFCntrSample* s = (SFCntrSample*) sample;

		char key[16];
		sprintf(key, "%i.%i.%i.%i",
				((s->agent_address & 0xff000000) >> 24),
				((s->agent_address & 0x00ff0000) >> 16),
				((s->agent_address & 0x0000ff00) >> 8),
				(s->agent_address & 0x000000ff)
			   );
		uint32_t id = cmph_search(h, key, strlen(key));
		agent_t* a = agent_get(agents, id); 

		if((uint32_t)s->timestamp/60 != a->fd_min_cntr){
			logmsg(LOGLEVEL_DEBUG, "Updating counter file descriptor for %s", key);
			a->fd_min_cntr = s->timestamp/60;

			// Generate the path to the next file
			char filename[256];
			memset(filename, 0, 256);
			getFilePath(s->agent_address, s->timestamp, filename);

			// filename is the name of next data segment
			sprintf(filename+strlen(filename), "cntr");

			if(a->fd_cntr != 0){
				msg_t m;	
				memset(&m, 0, sizeof(msg_t));
				m.agent = a->agent;
				m.type = SFTYPE_CNTR;
				m.timestamp = ((uint32_t)s->timestamp/60)-1;
				strncpy(m.filename, a->fn_cntr, 256);
				send_msg(queue, &m);
			}

			int f;
			if ( (f=shm_open(filename, O_CREAT | O_RDWR, S_IRWXU)) == -1){
				logmsg(LOGLEVEL_ERROR, "%s", strerror(errno));
			}

			a->fd_cntr = f;
			strncpy(a->fn_cntr, filename, 256);
			a->agent = s->agent_address;
		}
		write(a->fd_cntr, sample, sizeof(SFCntrSample));
	}
}