#ifndef __datasource_h__
#define __datasource_h__

#include <stdint.h>

typedef struct _ds_status {
	// The agent and interface is the unique identifier
	uint32_t agent;				// Agent address
	uint32_t interface;			// Interface index
	// Data fields for keep for each data source
	uint32_t last_seen;			// Last seen
	uint32_t sample_rate;		// Last sampling rate
	uint32_t sequence;			// Last sequence number
	uint32_t drops;				// Total number of drops from this data source
	struct _ds_status* next;
} ds_status_t;

void datasource_hash_init(ds_status_t** hash);
void datasource_hash_destroy(ds_status_t** hash);
ds_status_t* datasource_hash_lookup(ds_status_t** hash, uint32_t agent, uint32_t ifindex);

#endif
