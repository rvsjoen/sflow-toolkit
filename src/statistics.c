#include "statistics.h"

#define STAT_FILE 	"/proc/self/stat"
#define STAT_UTIME 	14
#define STAT_STIME 	15
#define STAT_VMEM 	23

int utime_prev;
int stime_prev;
int hz;

//TODO Filename can be calculated once

void get_HZ()
{
	long ticks;
	if ((ticks = sysconf(_SC_CLK_TCK)) == -1)
		logmsg(LOGLEVEL_ERROR, "Error gettings ticks from sysconf");
	hz = (unsigned int) ticks;
}

void init_stats(){
	get_HZ();
	char filename[256];
	sprintf(filename, "%s/statistics.rrd", cwd);
	int fd;
	if ((fd = open(filename, O_RDONLY)) == -1){
		char *createparams[] = {
				"rrdcreate",
				filename,
				"-s",
				"5",
		        "DS:samples:GAUGE:600:0:U",
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
		rrd_create(14, createparams);
	} else {
		close(fd);
	}
}

void update_stats(unsigned int samples, unsigned int seconds, uint32_t bytes){
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

	char filename[256];
	sprintf(filename, "%s/statistics.rrd", cwd);
	vmem >>= 10; // Convert to kilobytes from pages
	if(seconds != 0){
		char tmp[1024];
		sprintf(tmp, "%u:%u:%u:%u:%u", (unsigned int)time(NULL), samples, ((stime+utime)*100)/(hz*seconds), vmem, bytes);
		char *updateparams[] = {
			"rrdupdate",
			filename,
			tmp,
			NULL
		};
		optind = opterr = 0;
		logmsg(LOGLEVEL_DEBUG, "Updating stats: Time %u, Samples %u, CPU %u, VMEM %u, Bytes written %u", (unsigned int)time(NULL), samples, ((stime+utime)*100)/(hz*seconds), vmem, bytes);
		rrd_clear_error();
		rrd_update(3, updateparams);
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  update_realtime_stats
 *  Description:  Write some realtime stats to /<storagedir>/rstats
 * =====================================================================================
 */
void update_realtime_stats()
{
	char filename[256];
	sprintf(filename, "%s/rstats", cwd);
	FILE* fp = NULL;
	fp = fopen(filename, "w+");
	if(fp !=NULL){
		char buf[1024];
		sprintf(buf,"%u,%u,%u,%lu,%u", cnt, cnt_total_f, cnt_total_c, time(NULL) - time_start, bytes_total);
		fputs(buf, fp);
		fflush(fp);
		fclose(fp);
	}
}
