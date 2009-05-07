#include "agentlist.h"

agent_address_t** agenthash;

void agentlist_init()
{
	agenthash = (agent_address_t**) malloc(sizeof(agent_address_t*) * HASH_SIZE);
	memset(agenthash, 0, sizeof(agent_address_t*) * HASH_SIZE);
}

agent_t* agentlist_search(uint32_t addr)
{
	agent_t* result = NULL;
	uint32_t key = addr & (HASH_SIZE - 1);
	agent_address_t* agent_addr = agenthash[key];
	if(agent_addr != NULL){
		while(agent_addr->next != NULL && agent_addr->address != addr){
			agent_addr = agent_addr->next;
		}
	}
	return result;
}

agent_t* agentlist_add_agent(char* name, uint32_t address){
	agent_t* agent = (agent_t*) malloc(sizeof(agent_t));
	memset(agent, 0, sizeof(agent_t));
	strncpy(agent->name, name, 32);
	agent->address = address;
	return agent;
}

void agentlist_add_address(uint32_t address, agent_t* agent){
	uint32_t key = address & (HASH_SIZE - 1);
	agent_address_t* agent_addr = (agent_address_t*) malloc(sizeof(agent_address_t));
	agent_addr->address = agent->address;
	if(agenthash[key] != NULL){
		agent_address_t* tmp = agenthash[key];
		tmp->next = agent_addr;
	}
	agenthash[key] = agent_addr;
}

void agentlist_destroy(){
	free(agenthash);
	//TODO MORE CLEANUP HERE
}
