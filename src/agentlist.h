#ifndef __agentlist_h__
#define __agentlist_h__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

#define HASH_SIZE 0x01000000

typedef struct _agentlist_t {
	uint32_t num_agents;
	struct _agent_t* data;
} agentlist_t;

typedef struct _agent_t {
	char  name[32];
	uint32_t address;
	uint32_t last_seen;
	uint32_t sequence;
	uint64_t datagrams;	
	uint64_t drops;
	char fn_flow[256];
	char fn_cntr[256];
	int fd_flow;
	int fd_cntr;
	uint32_t fd_min_flow;
	uint32_t fd_min_cntr;
} agent_t;

typedef struct _agent_address_t {
	uint32_t address;
	struct agent_t* agent;
	struct _agent_address_t* next;
} agent_address_t;

void agentlist_init();
agent_t* agentlist_search(uint32_t addr);
agent_t* agentlist_add_agent(char* name, uint32_t address);
void agentlist_add_address(uint32_t address, agent_t* agent);
void agentlist_destroy();

/*
agentlist_t* agentlist_init(uint32_t num);
void agentlist_destroy(agentlist_t* list);
void agentlist_print_stats(agentlist_t* list);
agent_t* agent_get(agentlist_t* list, uint32_t index);
*/

#endif
