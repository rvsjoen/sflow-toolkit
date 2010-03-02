#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "statistics.h"
#include "messaging.h"
#include "logger.h"
#include "util.h"
#include "dataparser.h"
#include "configparser.h"

#include "storage.h"
#include "storage_mysql.h"
#include "storage_csv.h"
#include "storage_dummy.h"

extern stprocessd_config_t stprocessd_config;
extern stcollectd_config_t stcollectd_config;

extern uint32_t log_level;

bool daemonize 		= true;
bool debug_nostore 	= false;

mqd_t queue;

void print_config(){
	logmsg(LOGLEVEL_DEBUG, "Configuration: stprocessd:");
	logmsg(LOGLEVEL_DEBUG, "\tLoglevel: %u", stprocessd_config.loglevel);
	logmsg(LOGLEVEL_DEBUG, "\tHash size: %u", stprocessd_config.hashsize);
	logmsg(LOGLEVEL_DEBUG, "\tData dir: %s", stprocessd_config.datadir);
	logmsg(LOGLEVEL_DEBUG, "\tStats interval: %u seconds", stprocessd_config.stats_interval);
}

void parse_commandline(int argc, char** argv){
	int opt;
	while((opt = getopt(argc, argv, "vdn")) != -1){
		switch(opt)
		{
			case 'd': daemonize = false; 	break;
			case 'v': log_level++;			break;
			case 'n': debug_nostore = true;	break;
		}
	}
}

void process_file(const msg_t* m){
	if(m->type == SFTYPE_FLOW){
		logmsg(LOGLEVEL_INFO, "Processing flow file (%s)", m->filename);
		process_file_flow(m->filename, m->agent, m->timestamp);
	} else if (m->type == SFTYPE_CNTR) {
		logmsg(LOGLEVEL_DEBUG, "Processing counter file (%s)", m->filename);
		process_file_cntr(m->filename, m->agent, m->timestamp);
	}
}

int main(int argc, char** argv){

	initLogger("stprocessd");

	parse_commandline(argc, argv);
	parse_config_file(NULL, argv[0]);
	print_config();

	stats_init_stprocessd();

	if(daemonize)
		daemonize_me();

	time_t start = time(NULL);

	// Initialize the storage system
	storage_system_init();
	
	// Load the active storage modules
	if(!debug_nostore){
		storage_mysql_load();
		storage_csv_load();
		storage_dummy_load();

		// Initialize each loaded storage module
		storage_modules_init();
	} else {
		logmsg(LOGLEVEL_INFO, "Running in nostore mode, skipping module loading");
	}

	logmsg(LOGLEVEL_INFO, "Initializing message queue");
	queue = open_msg_queue(stcollectd_config.msgqueue);

	if(queue != -1){
		msg_t m;
		while(true){
			time_t now = time(NULL);
			if((now-start)%stprocessd_config.stats_interval == 0)
				stats_update_stprocessd(now-start, queue);
			memset(&m, 0, sizeof(msg_t));
			recv_msg(queue, &m);
			process_file(&m);
		}
		close_msg_queue(queue);
	}

	if(!debug_nostore){
		storage_modules_destroy();
	}

	storage_system_destroy();
	destroyLogger();
	return EXIT_SUCCESS;
}
