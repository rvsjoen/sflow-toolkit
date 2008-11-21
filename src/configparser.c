#include "configparser.h"
#include "logger.h"

// Internal variables
static uint32_t config_print_interval;
static uint32_t config_num_buffers;
static uint32_t config_port;
static uint32_t config_flush_interval;
static uint32_t config_buffer_size;
static char* 	 config_interface;
static char* 	 config_datadir;
static char** 	 config_validagents;
static uint32_t config_num_agents;

bool is_value;
bool is_list;
char key[256];
agent_node* agent_list;

void get_agents(){
	// First we just count the elements in the linked list to allocate some memory
	uint32_t num = 0;
	agent_node* start = agent_list;
	while(start != NULL){
		num++;
		start = start->next;
	}

	// If we are re-reading the list free the old list first
	if (config_validagents != NULL)
	{
		uint32_t j;
		for(j=0;j<config_num_agents;j++)
			free(config_validagents[j]);
		free(config_validagents);
		config_validagents = NULL;
	}

	char** result = NULL;
	result = malloc(sizeof(char*)*num);
	memset(result, 0, sizeof(char*)*num);
	
	// Now we allocate memory for each agent and free the memory allocated for the struct
	start = agent_list;
	int i = 0;

	while(start != NULL){
		result[i] = malloc(sizeof(char)*strlen(start->agent)+1);
		memset(result[i], 0, sizeof(char)*strlen(start->agent)+1);
		strncpy(result[i], start->agent, strlen(start->agent));
		agent_node* tmp = start;
		start = start->next;
		free(tmp);
		i++;
	}

	// Update these values so the getters can get to the data
	config_num_agents = num;
	config_validagents = result;
}

void parse_event(const yaml_event_t ev){
	yaml_event_type_t ev_type = ev.type;
	switch(ev_type)
	{
		case YAML_SCALAR_EVENT: 
			if(!is_value){
				strncpy(key, (char*) ev.data.scalar.value, ev.data.scalar.length);
				key[ev.data.scalar.length] = '\0';
				is_value = !is_value;
			} else if(is_list) {
				if (strcmp(key, CONFIG_KEY_AGENTS) == 0){
					if(agent_list == 0) {
						agent_node* tmp = (agent_node*)malloc(sizeof(agent_node));
						tmp->next = 0;
						strncpy(tmp->agent, (char*) ev.data.scalar.value, ev.data.scalar.length);
						agent_list = tmp;
					} else {
						agent_node* tmp = malloc(sizeof(agent_node));
						tmp->next = agent_list;
						strncpy(tmp->agent, (char*) ev.data.scalar.value, ev.data.scalar.length);
						agent_list = tmp;	
					}
				}
			} else {
				char* val = (char*) ev.data.scalar.value;
				if(strcmp(key, CONFIG_KEY_FLUSH_INTERVAL)==0) {
					config_flush_interval = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_PRINT_INTERVAL) == 0) {
					config_print_interval = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_INTERFACE) == 0){
					if(config_interface != NULL)
						free(config_interface);
					config_interface = malloc(sizeof(char)*strlen(val)+1);
					strncpy(config_interface, val, strlen(val));
				} else if (strcmp(key, CONFIG_KEY_PORT) == 0) {
					config_port = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_DATA_DIR) == 0) {
					if(config_datadir != NULL)
						free(config_datadir);
					config_datadir = malloc(sizeof(char)*strlen(val)+1);
					strncpy(config_datadir, val, strlen(val));
				} else if (strcmp(key, CONFIG_KEY_BUFFER_SIZE) == 0) {
					config_buffer_size = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_BUFFER_COUNT) == 0) {
					config_num_buffers = atoi(val);
				} 
				is_value = !is_value;
			}
			break;
		case YAML_SEQUENCE_START_EVENT:
			is_list = true;
			break;
		case YAML_SEQUENCE_END_EVENT:
			is_list = false;
			is_value = false;
			break;
		default:
			break;
	}
}

void parse_config_file(char* filename){
	agent_list = NULL;
	is_list = false;
	is_value = false;
	FILE *file = NULL;
	yaml_parser_t parser;
	yaml_event_t event;
	int done = 0;
	int count = 0;
	int error = 0;
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
			parse_event(event);
			done = (event.type == YAML_STREAM_END_EVENT);
			yaml_event_delete(&event);
			count ++;
		}
		yaml_parser_delete(&parser);
		fclose(file);
		get_agents();
		logmsg(LOGLEVEL_DEBUG, "Parsing complete: %s (%d events)", (error ? "FAILURE" : "SUCCESS"), count);
	} else {
		logmsg(LOGLEVEL_ERROR, "Error reading configuration file %s", filename);
	}
}

char* config_get_datadir(){
	return config_datadir;
}

char* config_get_interface(){
	return config_interface;
}

uint32_t config_get_print_interval(){ 
	return config_print_interval; 
}

uint32_t config_get_flush_interval(){ 
	return config_flush_interval;
}

uint32_t config_get_buffer_size(){ 
	return config_buffer_size; 
}

uint32_t config_get_num_buffers(){ 
	return config_num_buffers; 
}

uint32_t config_get_num_agents(){ 
	return config_num_agents; 
}

uint32_t config_get_port(){ 
	return config_port; 
}

char** config_get_validagents(){
	return config_validagents;
}
