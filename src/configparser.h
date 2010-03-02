#ifndef __configparser_h__
#define __configparser_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#define DEFAULT_CONFIG_FILE "/etc/sflow-toolkit.conf"

// Configuration values for stcollectd
typedef struct stcollectd_config {
	uint32_t port;				// Port to listen on
	uint32_t loglevel;			// Log level
	uint32_t hashbits;			// How many bits to use for the agent hash
	uint32_t stats_interval;	// How many seconds between statistics
	char  	 interface[16];		// Interface to bind to
	char  	 msgqueue[256];		// Name of the posix mqueue
	char  	 datadir[256];		// Where we put collected data
} stcollectd_config_t;

// Configuration values for stprocessd
typedef struct _stprocessd_config {
	uint32_t loglevel;			// Log level
	uint32_t hashsize;			// Size of the hash table we use
	char  	 datadir[256];		// Where we read the collected data
	uint32_t stats_interval;	// How many seconds between statistics
} stprocessd_config_t;

// Configuration values for storage_mysql
typedef struct _stprocessd_mysql_config {
	bool enabled;
	uint32_t interval;			// Interval size in minutes
	uint32_t num_intervals;		// Number of intervals to keep
	char  username[256];		// MySQl username
	char  password[256];		// MySQL password
	char  database[256];		// MySQL database
	char  hostname[256];		// MySQL hostname
	char  datadir[256];			// Where we store files used by LOAD INFILE
} stprocessd_mysql_config_t;

// Configuration values for storage_spectrum
typedef struct _stprocessd_spectrum_config {
	bool enabled;
	uint32_t interval;			// How often we switch logfiles (in seconds)
	uint32_t hashbits;			// How many bits to use for the agent hash
	char  	 datadir[256];		// Where to store the data
} stprocessd_spectrum_config_t;

void parse_event(const yaml_event_t ev, char* pname);
void parse_config_file(char* filename, char* pname);

#endif
