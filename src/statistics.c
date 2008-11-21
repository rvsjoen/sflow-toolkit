#include "messaging.h"
#include "configparser.h"
#include "statistics.h"

#define STAT_FILE 	"/proc/self/stat"
#define STAT_UTIME 	14
#define STAT_STIME 	15
#define STAT_VMEM 	23

uint32_t utime_prev;
uint32_t stime_prev;
uint32_t hz;

char stats_stcollectd_file[256];
char stats_stprocessd_file[256];
char stats_stcollectd_realtime_file[256];

void get_HZ(){
	long ticks;
	if ((ticks = sysconf(_SC_CLK_TCK)) == -1)
		logmsg(LOGLEVEL_ERROR, "Error gettings ticks from sysconf");
	hz = (uint32_t) ticks;
}

void stats_init_stprocessd(){
	get_HZ();
	memset(stats_stprocessd_file, 0, 256);
	sprintf(stats_stprocessd_file, "%s/statistics_stprocessd.rrd", config_get_datadir());
	int fd;
	if ((fd = open(stats_stprocessd_file, O_RDONLY)) == -1){
		char *createparams[] = {
				"rrdcreate",
				stats_stprocessd_file,
				"-s",
				"5",
		        "DS:cpu:GAUGE:600:0:100",
		        "DS:mem:GAUGE:600:0:U",
		        "DS:messages:GAUGE:600:0:U",
		        "RRA:AVERAGE:0.5:1:720",
		        "RRA:AVERAGE:0.5:6:720",
		        "RRA:AVERAGE:0.5:120:720",
		        "RRA:AVERAGE:0.5:840:720",
		        "RRA:AVERAGE:0.5:3270:720",
				NULL
		};
		optind = opterr = 0; // Because rrdtool uses getopt()
		rrd_clear_error();
		rrd_create(12, createparams);
	} else {
		close(fd);
	}
}

void stats_init_stcollectd(){
	get_HZ();
	memset(stats_stcollectd_file, 0, 256);
	memset(stats_stcollectd_realtime_file, 0, 256);
	sprintf(stats_stcollectd_file, "%s/statistics_stcollectd.rrd", config_get_datadir());
	sprintf(stats_stcollectd_realtime_file, "%s/statistics_stcollectd_realtime", config_get_datadir());
	int fd;
	if ((fd = open(stats_stcollectd_file, O_RDONLY)) == -1){
		char *createparams[] = {
				"rrdcreate",
				stats_stcollectd_file,
				"-s",
				"5",
		        "DS:datagrams:COUNTER:600:0:U",
		        "DS:samples:COUNTER:600:0:U",
				"DS:samples_flow:COUNTER:600:0:U",
				"DS:samples_cntr:COUNTER:600:0:U",
				"DS:agents:GAUGE:600:0:U",
		        "DS:cpu:GAUGE:600:0:100",
		        "DS:mem:GAUGE:600:0:U",
		        "DS:write:GAUGE:600:0:U",
		        "RRA:AVERAGE:0.5:1:720",
		        "RRA:AVERAGE:0.5:6:720",
		        "RRA:AVERAGE:0.5:120:720",
		        "RRA:AVERAGE:0.5:840:720",
		        "RRA:AVERAGE:0.5:3270:720",
				NULL
		};
		optind = opterr = 0; // Because rrdtool uses getopt()
		rrd_clear_error();
		rrd_create(17, createparams);
	} else {
		close(fd);
	}
}

void stats_update_stprocessd(uint32_t seconds, mqd_t queue){
	unsigned int vmem 		= 0;
	unsigned int utime		= 0;
	unsigned int stime		= 0;

	int fd;
	if ((fd = open(STAT_FILE, O_RDONLY)) == -1){
		logmsg(LOGLEVEL_ERROR,strerror(errno));
	}
	char str[1024];
	memset(str, 0, 1024);
	read(fd, str, 1024);

	char delims[] = " ";
	int i = 1;
	char *result = NULL;
	result = strtok( str, delims );
	while( result != NULL ) {
		switch(i){
			case STAT_UTIME: 
				utime = atoi(result) - utime_prev;
				utime_prev = atoi(result);
				break;
			case STAT_STIME: 
				stime = atoi(result) - stime_prev;
				stime_prev = atoi(result);
				break;
			case STAT_VMEM: 
				vmem = atoi(result);
				break;
		}
		i++;
		result = strtok( NULL, delims );
	}
	close(fd);

	vmem >>= 10; // Convert to kilobytes from pages

	if(seconds != 0){
		char tmp[1024];
		sprintf(tmp, "%u:%u:%u:%u", 
				(uint32_t)time(NULL), 
				((stime+utime)*100)/(hz*seconds), 
				vmem,
				msg_pending(queue)
				);
		char *updateparams[] = {
			"rrdupdate",
			stats_stcollectd_file,
			tmp,
			NULL
		};
		optind = opterr = 0;
		rrd_clear_error();
		rrd_update(3, updateparams);
	}
}

void stats_update_stcollectd(uint32_t seconds, uint32_t num_agents, uint64_t total_datagrams, uint64_t total_samples_flow, uint64_t total_samples_cntr, uint64_t total_bytes_written){
	unsigned int vmem 		= 0;
	unsigned int utime		= 0;
	unsigned int stime		= 0;

	int fd;
	if ((fd = open(STAT_FILE, O_RDONLY)) == -1){
		logmsg(LOGLEVEL_ERROR,strerror(errno));
	}
	char str[1024];
	memset(str, 0, 1024);
	read(fd, str, 1024);

	char delims[] = " ";
	int i = 1;
	char *result = NULL;
	result = strtok( str, delims );
	while( result != NULL ) {
		switch(i){
			case STAT_UTIME: 
				utime = atoi(result)-utime_prev;
				utime_prev = atoi(result);
				break;
			case STAT_STIME: 
				stime = atoi(result) - stime_prev;
				stime_prev = atoi(result);
				break;
			case STAT_VMEM: 
				vmem = atoi(result);
				break;
		}
		i++;
		result = strtok( NULL, delims );
	}
	close(fd);

	vmem >>= 10; // Convert to kilobytes from pages

	if(seconds != 0){
		char tmp[1024];
		sprintf(tmp, "%u:%llu:%llu:%llu:%llu:%u:%u:%u:%llu", 
				(uint32_t)time(NULL), 
				total_datagrams,
				total_samples_flow+total_samples_cntr,
				total_samples_flow,
				total_samples_cntr,
				num_agents,
				((stime+utime)*100)/(hz*seconds), 
				vmem, 
				total_bytes_written
				);
		char *updateparams[] = {
			"rrdupdate",
			stats_stcollectd_file,
			tmp,
			NULL
		};
		optind = opterr = 0;
		rrd_clear_error();
		rrd_update(3, updateparams);
	}
}

void stats_update_stcollectd_realtime(uint32_t time_start, uint32_t num_agents, uint64_t total_datagrams, uint64_t total_samples_flow, uint64_t total_samples_cntr, uint64_t total_bytes_written){
	FILE* fp = NULL;
	fp = fopen(stats_stcollectd_realtime_file, "w+");
	if(fp !=NULL){
		char buf[1024];
		sprintf(buf,"%llu,%llu,%llu,%u,%llu,%u", total_datagrams, total_samples_flow, total_samples_cntr, time_start, total_bytes_written, num_agents);
		fputs(buf, fp);
		fflush(fp);
		fclose(fp);
	}
}
