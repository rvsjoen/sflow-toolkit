#ifndef __configparser_h__
#define __configparser_h__

#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define CONFIG_KEY_FLUSH_INTERVAL	"flush interval"
#define CONFIG_KEY_PRINT_INTERVAL	"print interval"
#define CONFIG_KEY_INTERFACE		"interface"
#define CONFIG_KEY_PORT				"port"
#define CONFIG_KEY_DATA_DIR			"data directory"
#define CONFIG_KEY_AGENTS			"agents"
#define CONFIG_KEY_BUFFER_SIZE		"buffer size"
#define CONFIG_KEY_BUFFER_COUNT		"buffer count"

typedef struct _agent_node {
	char agent[16];
	struct _agent_node* next;
} agent_node;

void get_agents();
void parse_event(const yaml_event_t ev);
void parseConfigFile(char* filename);

#endif
