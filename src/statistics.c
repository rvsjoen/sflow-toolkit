#include "statistics.h"

#define STAT_FILE 	"/proc/self/stat"
#define IO_FILE 	"/proc/self/io"
#define STAT_UTIME 	14
#define STAT_STIME 	15
#define STAT_VMEM 	22
#define IO_READ 	10
#define IO_WRITE 	12

int utime_prev;
int stime_prev;

void init_stats(){
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
		        "DS:disk_read:COUNTER:600:0:U",
		        "DS:disk_write:COUNTER:600:0:U",
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


void update_stats(unsigned int samples, unsigned int seconds){
	unsigned int vmem 		= 0;
	unsigned int utime		= 0;
	unsigned int stime		= 0;
	unsigned int b_read		= 0;
	unsigned int b_write	= 0;

	int fd;
	if ((fd = open(STAT_FILE, O_RDONLY)) == -1){
		printf(strerror(errno));
	}
	char str[1024];
	memset(str, 0, 1024);
	read(fd, str, 1024);

	char delims[] = " \n";
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
	if ((fd = open(IO_FILE, O_RDONLY)) == -1){
		printf(strerror(errno));
	}
	read(fd, str, 1024);
	i = 1;
	result = NULL;
	result = strtok( str, delims );
	while( result != NULL ) {
		switch(i){
			case IO_READ: 
				b_read = atoi(result);
				break;
			case IO_WRITE: 
				b_write = atoi(result);
				break;
		}
		i++;
		result = strtok( NULL, delims );
	}
	close(fd);
	char filename[256];
	sprintf(filename, "%s/statistics.rrd", cwd);

	if(seconds != 0){
		char tmp[1024];
		sprintf(tmp, "%u:%u:%u:%u:%u:%u", (unsigned int)time(NULL), samples, ((stime+utime)*100)/(HZ*seconds), vmem, b_read, b_write);
		char *updateparams[] = {
			"rrdupdate",
			filename,
			tmp,
			NULL
		};
		optind = opterr = 0;
		//printf("%s\n", tmp); //TODO Remove this 
		rrd_clear_error();
		rrd_update(3, updateparams);
	}
}
