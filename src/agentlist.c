#include "agentlist.h"

agentlist_t* agentlist_init(uint32_t num){
	agentlist_t* list = (agentlist_t*) malloc(sizeof(agentlist_t));
	memset(list, 0, sizeof(agentlist_t));
	list->num_agents = num;
	list->data = (agent_t*) malloc(num*sizeof(agent_t));
	memset(list->data, 0, num * sizeof(agent_t));
	return list;
}

void agentlist_destroy(agentlist_t* list){
	free(list->data);
}

agent_t* agent_get(agentlist_t* list, uint32_t index){
	return &(list->data[index]);
}

void agentlist_print_stats(agentlist_t* list){
	uint32_t i;
	logmsg(LOGLEVEL_DEBUG, "Agent Stats (%u agents):", list->num_agents);
	for( i=0; i<list->num_agents; i++ )
	{
		agent_t* as = agent_get(list, i);
		logmsg(LOGLEVEL_DEBUG, "agent[%u] %s received %u datagrams ", as->index, as->address, as->datagrams);
	}
}
