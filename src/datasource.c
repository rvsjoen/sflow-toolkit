#include <string.h>
#include <stdlib.h>
#include "datasource.h"

#define HASH_SIZE 0x01000000 // 24 bit

void datasource_hash_init(ds_status_t*** hash){
	// Allocate a pointer for each entry in the hash, initialize it to 0
	*hash = (ds_status_t**) malloc(sizeof(ds_status_t*)*HASH_SIZE);
	memset(*hash, 0, sizeof(ds_status_t*)*HASH_SIZE);
}

void datasource_hash_destroy(ds_status_t** hash){
	free(hash);
	//TODO Cleanup
}

ds_status_t* datasource_hash_lookup(ds_status_t** hash, uint32_t agent, uint32_t ifindex){
	uint32_t key = agent & (HASH_SIZE - 1);
	ds_status_t* s;

	// If this hash bucket has no entries from before, create the first one
	// Else loop through the entries until we find what we want
	if(hash[key] == 0){
		hash[key] = (ds_status_t*) malloc(sizeof(ds_status_t));
		memset(hash[key], 0, sizeof(ds_status_t));
		s = hash[key];
		// Populate the fields with initial values
		s->agent 		= agent;
		s->interface 	= ifindex;
	} else {
		ds_status_t* ptr = hash[key];

		// Search for the correct datasource in the linked list
		while(ptr != NULL && !(ptr->agent == agent && ptr->interface == ifindex))
			ptr = ptr->next;

		// No hits ? Allocate a new datasource structure and initialize it
		if (ptr == NULL){
			ptr = (ds_status_t*) malloc(sizeof(ds_status_t));
			memset(ptr, 0, sizeof(ds_status_t));
			ptr->agent = agent;
			ptr->interface = ifindex;
			ptr->next = hash[key];
			hash[key] = ptr;
		}
		s = ptr;
	}
	return s;
}
