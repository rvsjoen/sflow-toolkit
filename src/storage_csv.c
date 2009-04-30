#include "storage.h"
#include "storage_csv.h"

#define HASH_SIZE	0x01000000 // 24 bit

time_t storage_csv_time;
uint32_t storage_csv_fd = 0;

typedef struct _cntr_status {
	uint32_t agent;
	uint32_t interface;
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

cntr_status_t* hash_lookup(uint32_t agent, uint32_t ifindex){
	uint32_t key = agent & (HASH_SIZE - 1);
	cntr_status_t* s;

	// If this hash bucket has no entries from before, create the first one
	// Else loop through the entries until we find what we want
	//
	
	if(cntr_status_hash[key] == 0){
		cntr_status_hash[key] = (cntr_status_t*) malloc(sizeof(cntr_status_t));
		memset(cntr_status_hash[key], 0, sizeof(cntr_status_t));
		s = cntr_status_hash[key];
		s->agent = agent;
		s->interface = ifindex;
	} else {
		cntr_status_t* ptr = cntr_status_hash[key];

		while(ptr != NULL && !(ptr->agent == agent && ptr->interface == ifindex))
			ptr = ptr->next;

		if (ptr == NULL){
			ptr = (cntr_status_t*) malloc(sizeof(cntr_status_t));
			memset(ptr, 0, sizeof(cntr_status_t));
			ptr->agent = agent;
			ptr->interface = ifindex;
			ptr->next = cntr_status_hash[key];
			cntr_status_hash[key] = ptr;
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

		cntr_status_t* cstat = hash_lookup(s->agent_address, s->counter_generic_if_index);

		char a[16];
		num_to_ip(s->agent_address, a);
		uint32_t num;

		// Save these values before we update cstat with the new values
		uint32_t d_time 		= abs(s->timestamp - cstat->timestamp);
		uint32_t d_in_octets 	= s->counter_generic_if_in_octets - cstat->octets_in;
		uint32_t d_out_octets 	= s->counter_generic_if_out_octets - cstat->octets_out;
		uint32_t d_linespeed 	= cstat->linespeed;

		double loadin, loadout, mbytesin, mbytesout;
		if(cstat->timestamp != 0){
			mbytesin	= d_in_octets / (d_time * 1e6);
			mbytesout	= d_out_octets / (d_time * 1e6);
			if(d_linespeed != 0){
				loadin 	= (d_in_octets  / (d_time * (d_linespeed/8.0))) * 100;
				loadout = (d_out_octets / (d_time * (d_linespeed/8.0))) * 100;
			} else {
				loadin 	= 0;
				loadout = 0;
			}
		} else {
			// This is the first counter sample, we can't calculate any values
			// since we also need the previous values, so just use zero
			mbytesin 	= 0;
			mbytesout 	= 0;
			loadin 		= 0;
			loadout 	= 0;
		}

		num = sprintf(buf, "%s,%u,%u,%u,%u,%f,%u,%u,%f,%u,%f,%f,%u,%u\n",
					a,
					s->counter_generic_if_index,
					(uint32_t)s->timestamp,
					s->counter_generic_if_out_discards,
					s->counter_generic_if_out_errors,
					loadout,
					s->counter_generic_if_in_ucast_pkts + s->counter_generic_if_in_mcast_pkts + s->counter_generic_if_in_bcast_pkts,
					s->counter_generic_if_out_ucast_pkts + s->counter_generic_if_out_mcast_pkts + s->counter_generic_if_out_bcast_pkts,
					loadin,
					s->counter_generic_if_in_discards,
					mbytesout,
					mbytesin,
					s->counter_generic_if_in_errors,
					s->counter_generic_if_speed/1000000
		);

		cstat->timestamp 	= s->timestamp;
		cstat->octets_in 	= s->counter_generic_if_in_octets;
		cstat->octets_out 	= s->counter_generic_if_out_octets;
		cstat->linespeed 	=  s->counter_generic_if_speed;

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
