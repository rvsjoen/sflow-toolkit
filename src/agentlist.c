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

	// If this hash lookup has elements and the first one does not match, 
	// try to find the correct agent by doing a linear search
	if(agent_addr != NULL){
		if(agent_addr->address == addr)
			result = agent_addr->agent;
		else {
			while(agent_addr->next != NULL && result != NULL){
				agent_addr = agent_addr->next;
				if(agent_addr->address == addr)
					result = agent_addr->agent;
			}
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
	memset(agent_addr, 0, sizeof(agent_address_t));
	agent_addr->address = address;
	agent_addr->agent = agent;

	logmsg(LOGLEVEL_DEBUG, "\t\tAdding %u to %x", address, agent);

	// Insert a new element at the head of the list if it's not empty
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
