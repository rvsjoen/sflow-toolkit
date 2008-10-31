#ifndef __agentlist_h__
#define __agentlist_h__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

typedef struct _agentlist_t {
	uint32_t num_agents;
	struct _agent_t* data;
} agentlist_t;

typedef struct _agent_t {
	char address[16];
	uint32_t index;
	uint32_t last_seen;
	uint64_t datagrams;	
	uint64_t samples_flow;
	uint64_t samples_counter;
	FILE* fd_flow;
	FILE* fd_cntr;
	uint32_t fd_min_flow;
	uint32_t fd_min_cntr;
} agent_t;

agentlist_t* agentlist_init(uint32_t num);
void agentlist_destroy(agentlist_t* list);
void agentlist_print_stats(agentlist_t* list);
agent_t* agent_get(agentlist_t* list, uint32_t index);

#endif
