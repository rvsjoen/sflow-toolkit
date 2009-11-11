#ifndef __agentlist_h__
#define __agentlist_h__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

#define HASH_SIZE 0x01000000

/*-----------------------------------------------------------------------------
 *  This structure is used to keep track of each agent, it contains
 *  only agent-specific information, one agent can sample from multiple
 *  interfaces.
 *-----------------------------------------------------------------------------*/
typedef struct _agent_t {
    char        name[32];       // Agent name
    uint32_t    address;        // Agent address
    uint32_t    last_seen;      // Last sample received from agent
    uint32_t    uptime;         // Agent uptime
    uint32_t    sequence;       // Sequence number of last sample received
    uint64_t    datagrams;      // Number of datagrams from this agent
    uint64_t    drops;          // Number of dropped datagrams
    char        fn_flow[256];   // Outfile for flow samples
    char        fn_cntr[256];   // Outfile for cntr samples
    int         fd_flow;        // File descriptor for flow samples
    int         fd_cntr;        // File descriptor for cntr samples
    uint32_t    fd_min_flow;    // Current minute timestamp
    uint32_t    fd_min_cntr;    // Current minute timestamp
} agent_t;

/*-----------------------------------------------------------------------------
 *  A linked list of addresses
 *-----------------------------------------------------------------------------*/
typedef struct _agent_address_t {
    uint32_t address;
    agent_t* agent;
    struct _agent_address_t* next;
} agent_address_t;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  agentlist_init
 *  Description:  Initialize the agent list data structures
 * =====================================================================================
 */
void agentlist_init();

agent_t* agentlist_search(uint32_t addr);

agent_t* agentlist_add_agent(char* name, uint32_t address);

void agentlist_add_address(uint32_t address, agent_t* agent);

void agentlist_destroy();

#endif
