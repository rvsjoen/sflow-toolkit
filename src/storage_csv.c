#include "storage.h"
#include "storage_csv.h"

#define HASH_SIZE	0x01000000 // 24 bit

time_t storage_csv_time;
uint32_t storage_csv_fd = 0;

typedef struct _cntr_status {
	uint32_t agent;
	uint32_t timestamp;
	uint64_t octets_in;
	uint64_t octets_out;
	uint32_t linespeed;
	struct _cntr_status* next;
} cntr_status_t;

cntr_status_t** cntr_status_hash;

void hash_init(){
	// Allocate a pointer for each entry in the hash, initialize it to 0
	cntr_status_hash = (cntr_status_t**) malloc(sizeof(cntr_status_t*)*HASH_SIZE);
	memset(cntr_status_hash, 0, sizeof(cntr_status_t*)*HASH_SIZE);
}

cntr_status_t* hash_lookup(uint32_t agent){
	uint32_t key = agent & (HASH_SIZE - 1);
	cntr_status_t* s;

	// If this hash bucket has no entries from before, create the first one
	// Else loop through the entries until we find what we want
	if(cntr_status_hash[key] == 0){
		cntr_status_hash[key] = (cntr_status_t*) malloc(sizeof(cntr_status_t));
		memset(cntr_status_hash[key], 0, sizeof(cntr_status_t));
		s = cntr_status_hash[key];
	} else {
		cntr_status_t* ptr = cntr_status_hash[key];
		while(ptr != 0 && ptr->agent != agent)
			ptr = ptr->next;

		if (ptr == 0){
			ptr = (cntr_status_t*) malloc(sizeof(cntr_status_t));
			memset(ptr, 0, sizeof(cntr_status_t));
		}
		s = ptr;
	}
	return s;
}

void storage_csv_store_cntr(counter_list_t* list, uint32_t timestamp){
	UNUSED_ARGUMENT(timestamp);

	// If we have no file handle
	if (storage_csv_fd == 0){
		char filename[32];

		struct tm* tmp;
		time_t t = time(NULL);
		tmp = localtime(&t);
		strftime(filename, 32, "SFlow%Y%m%d_%H_%M.log", tmp);

		storage_csv_fd = shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		storage_csv_time = time(NULL);
	}

	// If time is expired we need to create a new file handle
	if( (time(NULL) - storage_csv_time) >= (60*5)){
		close(storage_csv_fd);
		char filename[32];

		struct tm* tmp;
		time_t t = time(NULL);
		tmp = localtime(&t);
		strftime(filename, 32, "SFlow%Y%m%d_%H_%M.log", tmp);

		storage_csv_fd = shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		storage_csv_time = time(NULL);
	}

	char buf[1024];
	counter_list_node_t* node = list->data;
	while(node != NULL){
		SFCntrSample* s = &node->sample;

		cntr_status_t* cstat = hash_lookup(s->agent_address);

		char a[16];
		num_to_ip(s->agent_address, a);
		uint32_t num;

		num = sprintf(buf, "%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
					a,
					s->counter_generic_if_index,
					(uint32_t)s->timestamp,
					s->counter_generic_if_out_discards,
					s->counter_generic_if_out_errors,
					0, //loadout
					s->counter_generic_if_in_ucast_pkts + s->counter_generic_if_in_mcast_pkts + s->counter_generic_if_in_bcast_pkts,
					s->counter_generic_if_out_ucast_pkts + s->counter_generic_if_out_mcast_pkts + s->counter_generic_if_out_bcast_pkts,
					0, //loadin
					s->counter_generic_if_in_discards,
					0, //mbytesout
					0, //mbytesin
					s->counter_generic_if_in_errors
		);

		write(storage_csv_fd, buf, num);
		node = node->next;

		}
	}

// This module only cares about counter samples so we only implement store_cntr
storage_module_t storage_mod_csv = {
	.name 					= "csv",
	.init 					= NULL,
	.destroy 				= NULL,
	.store_cntr 			= storage_csv_store_cntr,
	.store_conv_ethernet 	= NULL,
	.store_conv_ip 			= NULL,
	.store_conv_tcp 		= NULL,
	.store_conv_udp 		= NULL
};

void storage_csv_load(){
	// Initialize the hash
	hash_init();

	// Finally register the module with the rest of the storage system
	logmsg(LOGLEVEL_DEBUG, "Registering csv module");
	storage_modules_register(&storage_mod_csv);
}
