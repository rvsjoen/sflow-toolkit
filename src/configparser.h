#ifndef __configparser_h__
#define __configparser_h__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

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
void parse_config_file(char* filename);
char* config_get_datadir();
char* config_get_interface();
uint32_t config_get_print_interval();
uint32_t config_get_flush_interval();
uint32_t config_get_buffer_size();
uint32_t config_get_num_buffers();
uint32_t config_get_num_agents();
uint32_t config_get_port();
char** config_get_validagents();

#endif
