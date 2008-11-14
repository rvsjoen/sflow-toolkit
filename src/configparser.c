#include "configparser.h"
#include "logger.h"

extern uint32_t print_interval;
extern uint32_t num_buffers;
extern uint32_t port;
extern uint32_t flush_interval;
extern uint32_t buffer_size;
extern int32_t num_agents;
extern char* interface;
extern char** validagents;
extern char* cwd;

bool is_value;
bool is_list;
char key[256];
agent_node* agent_list;

void get_agents(){
	// First we just count the elements in the linked list to allocate some memory
	int num = 0;
	agent_node* start = agent_list;
	while(start != NULL){
		num++;
		start = start->next;
	}

	// If we are re-reading the list free the old list first
	if (validagents != NULL)
	{
		int j;
		for(j=0;j<num_agents;j++)
			free(validagents[j]);
		free(validagents);
		validagents = NULL;
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

	num_agents = num;
	validagents = result;
}

void parse_event(const yaml_event_t ev)
{
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
					flush_interval = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_PRINT_INTERVAL) == 0) {
					print_interval = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_INTERFACE) == 0){
					if(interface != NULL)
						free(interface);
					interface = malloc(sizeof(char)*strlen(val)+1);
					strncpy(interface, val, strlen(val));
				} else if (strcmp(key, CONFIG_KEY_PORT) == 0) {
					port = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_DATA_DIR) == 0) {
					if(cwd != NULL)
						free(cwd);
					cwd = malloc(sizeof(char)*strlen(val)+1);
					strncpy(cwd, val, strlen(val));
				} else if (strcmp(key, CONFIG_KEY_BUFFER_SIZE) == 0) {
					buffer_size = atoi(val);
				} else if (strcmp(key, CONFIG_KEY_BUFFER_COUNT) == 0) {
					num_buffers = atoi(val);
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

void parseConfigFile(char* filename)
{
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
