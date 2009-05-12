#include "configparser.h"
#include "agentlist.h"
#include "util.h"
#include "logger.h"

#define CONFIG_KEY_STCOLLECTD 					"stcollectd"
#define CONFIG_KEY_STPROCESSD 					"stprocessd"
#define CONFIG_KEY_STPROCESSD_STORAGE_SPECTRUM 	"storage_spectrum"
#define CONFIG_KEY_STPROCESSD_STORAGE_MYSQL 	"storage_mysql"
#define CONFIG_KEY_AGENTS						"agents"

#define CONFIG_KEY_STCOLLECTD_LOGLEVEL			"loglevel"
#define CONFIG_KEY_STCOLLECTD_PORT				"port"
#define CONFIG_KEY_STCOLLECTD_HASHBITS			"hash bits"
#define CONFIG_KEY_STCOLLECTD_PRINT_INTERVAL	"print interval"
#define CONFIG_KEY_STCOLLECTD_FLUSH_INTERVAL	"flush interval"
#define CONFIG_KEY_STCOLLECTD_BUFFER_SIZE		"buffer size"
#define CONFIG_KEY_STCOLLECTD_BUFFER_NUM		"buffer num"
#define CONFIG_KEY_STCOLLECTD_INTERFACE			"interface"
#define CONFIG_KEY_STCOLLECTD_DATADIR			"datadir"
#define CONFIG_KEY_STCOLLECTD_TMPDIR			"tmpdir"
#define CONFIG_KEY_STCOLLECTD_MSGQUEUE			"msgqueue"
#define CONFIG_KEY_STCOLLECTD_STATS_INTERVAL	"stats interval"

#define CONFIG_KEY_STPROCESSD_LOGLEVEL			"loglevel"
#define CONFIG_KEY_STPROCESSD_HASHSIZE			"hash size"
#define CONFIG_KEY_STPROCESSD_DATADIR			"datadir"
#define CONFIG_KEY_STPROCESSD_STATS_INTERVAL	"stats interval"

#define CONFIG_KEY_STORAGE_MYSQL_ENABLED		"enabled"
#define CONFIG_KEY_STORAGE_MYSQL_INTERVAL		"interval"
#define CONFIG_KEY_STORAGE_MYSQL_NUM_INTERVAL	"number of intervals"
#define CONFIG_KEY_STORAGE_MYSQL_USERNAME		"username"
#define CONFIG_KEY_STORAGE_MYSQL_PASSWORD		"password"
#define CONFIG_KEY_STORAGE_MYSQL_DATABASE		"database"
#define CONFIG_KEY_STORAGE_MYSQL_HOSTNAME		"hostname"
#define CONFIG_KEY_STORAGE_MYSQL_TMPDIR			"tmpdir"

#define CONFIG_KEY_STORAGE_SPECTRUM_ENABLED		"enabled"
#define CONFIG_KEY_STORAGE_SPECTRUM_INTERVAL	"interval"
#define CONFIG_KEY_STORAGE_SPECTRUM_DATADIR		"datadir"
#define CONFIG_KEY_STORAGE_SPECTRUM_HASHBITS	"hash bits"

// Structures with default values to contain the configuration values once the configuration 
// file is parsed
stcollectd_config_t	stcollectd_config = {
	.port			= 6343,
	.loglevel 		= 0,
	.hashbits 		= 24,
	.print_interval = 10000,
	.flush_interval	= 10,
	.stats_interval = 30,
	.buffer_size	= 10000,
	.buffer_num		= 2,
	.interface 		= "127.0.0.1",
	.datadir		= "data/",
	.tmpdir			= "tmp/",
	.msgqueue		= "/sflow"
};

stprocessd_config_t stprocessd_config = {
	.loglevel 		= 0,
	.hashsize 		= 10000,
	.stats_interval = 30,
	.datadir		= "data/",
};

stprocessd_mysql_config_t storage_mysql_config = {
	.enabled 		= false,
	.interval 		= 1440,
	.num_intervals	= 7,
	.username		= "sflow",
	.password		= "sflow",
	.database		= "sflow",
	.hostname		= "localhost",
	.tmpdir			= "tmp/"
};

stprocessd_spectrum_config_t storage_spectrum_config = {
	.enabled 		= false,
	.interval		= 300,
	.hashbits		= 24,
	.datadir		= "data/"
};

// Helper variables used while parsing
char key[256];
bool in_stcollectd;
bool in_stprocessd;
bool in_storage_spectrum;
bool in_storage_mysql;
bool in_agent_list;
bool in_agent;
bool is_value;
bool is_list;

agent_t* agent;

void parse_event(const yaml_event_t ev, char* pname){
	yaml_event_type_t ev_type = ev.type;
	switch(ev_type)
	{
		case YAML_SCALAR_EVENT: {
			char* val = (char*) ev.data.scalar.value;
			if(strcmp(val, CONFIG_KEY_STCOLLECTD) == 0){
				in_stcollectd = true;
				in_stprocessd = false;
			} else if (strcmp(val, CONFIG_KEY_STPROCESSD) == 0) {
				in_stcollectd = false;
				in_stprocessd = true;
			} else if (in_stprocessd && strcmp(val, CONFIG_KEY_STPROCESSD_STORAGE_MYSQL) == 0 ){
				in_storage_spectrum = false;
				in_storage_mysql = true;
			} else if (in_stprocessd && strcmp(val, CONFIG_KEY_STPROCESSD_STORAGE_SPECTRUM) == 0 ){
				in_storage_spectrum = true;
				in_storage_mysql = false;
			} else {
				if(is_list && strcmp(pname+(strlen(pname)-strlen("stprocessd")),"stprocessd") != 0){
					// Set this if we are entering the agent list
					if (strcmp(key, CONFIG_KEY_AGENTS) == 0)
						in_agent_list = true;
				
					// If we are currently not parsing an agent we copy
					// the name of the next agent into the key, if we are already
					// parsing an agents we just grab the next address
					if (!in_agent){
						logmsg(LOGLEVEL_DEBUG, "\tParsing agent: %s", val);
						in_agent = true;
						strncpy(key, (char*) ev.data.scalar.value, ev.data.scalar.length);
						key[ev.data.scalar.length] = '\0';
					} else {
						if(agent == NULL){
							agent = agentlist_add_agent(key, ip_to_num(val));
							logmsg(LOGLEVEL_DEBUG, "\t\taddress (pri): %s, %s", key, val);
						} else {
							logmsg(LOGLEVEL_DEBUG, "\t\taddress (sec): %s, %s", key, val);
						}
						agentlist_add_address(ip_to_num(val), agent);
					}

				} else if (!is_value){
					strncpy(key, (char*) ev.data.scalar.value, ev.data.scalar.length);
					key[ev.data.scalar.length] = '\0';
					is_value = true;
				} else {

					if(in_stcollectd) {

						if(strcmp(key, CONFIG_KEY_STCOLLECTD_LOGLEVEL) == 0)
							stcollectd_config.loglevel = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_PORT) == 0)
							stcollectd_config.port = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_HASHBITS) == 0)
							stcollectd_config.hashbits = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_PRINT_INTERVAL) == 0)
							stcollectd_config.print_interval = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_FLUSH_INTERVAL) == 0)
							stcollectd_config.flush_interval = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_BUFFER_SIZE) == 0)
							stcollectd_config.buffer_size = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_BUFFER_NUM) == 0)
							stcollectd_config.buffer_num = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_STATS_INTERVAL) == 0)
							stcollectd_config.stats_interval = atoi(val);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_INTERFACE) == 0)
							strncpy(stcollectd_config.interface, val, 16);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_DATADIR) == 0)
							strncpy(stcollectd_config.datadir, val, 256);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_TMPDIR) == 0)
							strncpy(stcollectd_config.tmpdir, val, 256);
						else if (strcmp(key, CONFIG_KEY_STCOLLECTD_MSGQUEUE) == 0)
							strncpy(stcollectd_config.msgqueue, val, 256);

					} else if (in_stprocessd) {

						if(in_storage_spectrum){
							if(strcmp(key, CONFIG_KEY_STORAGE_SPECTRUM_ENABLED) == 0)
								storage_spectrum_config.enabled = (atoi(val) == 1);
							else if(strcmp(key, CONFIG_KEY_STORAGE_SPECTRUM_INTERVAL) == 0)
								storage_spectrum_config.interval = atoi(val);
							else if(strcmp(key, CONFIG_KEY_STORAGE_SPECTRUM_HASHBITS) == 0)
								storage_spectrum_config.hashbits = atoi(val);
							else if (strcmp(key, CONFIG_KEY_STORAGE_SPECTRUM_DATADIR) == 0)
								strncpy(storage_spectrum_config.datadir, val, 256);

						} else if (in_storage_mysql){

							if(strcmp(key, CONFIG_KEY_STORAGE_MYSQL_ENABLED) == 0)
								storage_mysql_config.enabled = (atoi(val) == 1);
							else if(strcmp(key, CONFIG_KEY_STORAGE_MYSQL_INTERVAL) == 0)
								storage_mysql_config.interval = atoi(val);
							else if(strcmp(key, CONFIG_KEY_STORAGE_MYSQL_NUM_INTERVAL) == 0)
								storage_mysql_config.num_intervals = atoi(val);
							else if (strcmp(key, CONFIG_KEY_STORAGE_MYSQL_USERNAME) == 0)
								strncpy(storage_mysql_config.username, val, 256);
							else if (strcmp(key, CONFIG_KEY_STORAGE_MYSQL_PASSWORD) == 0)
								strncpy(storage_mysql_config.password, val, 256);
							else if (strcmp(key, CONFIG_KEY_STORAGE_MYSQL_DATABASE) == 0)
								strncpy(storage_mysql_config.database, val, 256);
							else if (strcmp(key, CONFIG_KEY_STORAGE_MYSQL_HOSTNAME) == 0)
								strncpy(storage_mysql_config.hostname, val, 256);
							else if (strcmp(key, CONFIG_KEY_STORAGE_MYSQL_TMPDIR) == 0)
								strncpy(storage_mysql_config.tmpdir, val, 256);

						} else {

							if(strcmp(key, CONFIG_KEY_STPROCESSD_LOGLEVEL) == 0)
								stprocessd_config.loglevel = atoi(val);
							else if(strcmp(key, CONFIG_KEY_STPROCESSD_HASHSIZE) == 0)
								stprocessd_config.hashsize = atoi(val);
							else if(strcmp(key, CONFIG_KEY_STPROCESSD_STATS_INTERVAL) == 0)
								stprocessd_config.stats_interval = atoi(val);
							else if (strcmp(key, CONFIG_KEY_STPROCESSD_DATADIR) == 0)
								strncpy(stprocessd_config.datadir, val, 256);
						}
					}
					is_value = false;
				}
			}
		} break;

		case YAML_SEQUENCE_START_EVENT: {
			is_list = true;
		} break;

		case YAML_SEQUENCE_END_EVENT: {
			if(in_agent) {
				logmsg(LOGLEVEL_DEBUG, "\tDone parsing agent: %s", key);
				agent = NULL;
				in_agent = false;
				is_value = false;
			} else if(in_agent_list){
				in_agent_list = false;
				is_list = false;
			} else {
				is_value = false;
			}
		} break;

		default:
			break;
	}
}

void parse_config_file(char* filename, char* pname){
	agent = NULL;
	is_list = false;
	is_value = false;
	FILE *file = NULL;
	yaml_parser_t parser;
	yaml_event_t event;
	uint32_t done = 0;
	uint32_t count = 0;
	uint32_t error = 0;
	file = fopen(filename, "r");
	if(file != NULL){
		logmsg(LOGLEVEL_INFO, "Reading configuration from file: %s", filename);
		yaml_parser_initialize(&parser);
		yaml_parser_set_input_file(&parser, file);
		while (!done)
		{
			if (!yaml_parser_parse(&parser, &event)) {
				error = 1;
				logmsg(LOGLEVEL_ERROR, "Error parsing configuration file");
				break;
			}
			parse_event(event, pname);
			done = (event.type == YAML_STREAM_END_EVENT);
			yaml_event_delete(&event);
			count++;
		}
		yaml_parser_delete(&parser);
		fclose(file);
		logmsg(LOGLEVEL_DEBUG, "Parsing complete: %s (%d events)", (error ? "FAILURE" : "SUCCESS"), count);
	} else {
		logmsg(LOGLEVEL_ERROR, "Error reading configuration file %s", filename);
	}
}
