#ifndef __configparser_h__
#define __configparser_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

// Configuration values for stcollectd
typedef struct stcollectd_config {
	uint32_t port;
	uint32_t loglevel;
	uint32_t hashbits;
	uint32_t print_interval;
	uint32_t flush_interval;
	uint32_t stats_interval;
	uint32_t buffer_size;
	uint32_t buffer_num;
	char  	 interface[16];
	char  	 datadir[256];
	char  	 tmpdir[256];
	char  	 msgqueue[256];
} stcollectd_config_t;

// Configuration values for stprocessd
typedef struct _stprocessd_config {
	uint32_t loglevel;
	uint32_t hashsize;
	uint32_t stats_interval;
	char  datadir[256];
} stprocessd_config_t;

// Configuration values for storage_mysql
typedef struct _stprocessd_mysql_config {
	bool enabled;
	uint32_t interval;
	uint32_t num_intervals;
	char  username[256];
	char  password[256];
	char  database[256];
	char  hostname[256];
	char  tmpdir[256];
} stprocessd_mysql_config_t;

// Configuration values for storage_spectrum
typedef struct _stprocessd_spectrum_config {
	bool enabled;
	uint32_t interval;
	uint32_t hashbits;
	char  datadir[256];
} stprocessd_spectrum_config_t;

void parse_event(const yaml_event_t ev, char* pname);
void parse_config_file(char* filename, char* pname);

#endif
