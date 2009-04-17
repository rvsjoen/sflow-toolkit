#include "storage.h"
#include "storage_csv.h"

time_t storage_csv_time;
uint32_t storage_csv_fd = 0;

void storage_csv_store_cntr(counter_list_t* list, uint32_t timestamp){
	UNUSED_ARGUMENT(timestamp);

	// If we have no file handle
	if (storage_csv_fd == 0){
		char filename[32];
		sprintf(filename, "counters_csv_%u", (uint32_t)time(NULL));
		storage_csv_fd = shm_open(filename, O_RDWR | O_CREAT, S_IRWXU);
		storage_csv_time = time(NULL);
	}

	// If time is expired we need to create a new file handle
	if( (time(NULL) - storage_csv_time) >= (60*5)){
		close(storage_csv_fd);
		char filename[32];
		sprintf(filename, "counters_csv_%u", (uint32_t)time(NULL));
		storage_csv_fd = shm_open(filename, O_RDWR | O_CREAT, S_IRWXU);
		storage_csv_time = time(NULL);
	}

	char buf[1024];
	counter_list_node_t* node = list->data;
	while(node != NULL){
		SFCntrSample* s = &node->sample;
		char a[16];
		num_to_ip(s->agent_address, a);
		uint32_t num;

		num = sprintf(buf, "%s,%u,%u,%u,%u,%u,%u,%u,%u,%u,%llu,%llu,%u",
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

// Device_IP,interface_index,timestamp,ifOutDiscards,ifOutErrors,LoadOut,if
// InPackets,ifOutPackets,LoadIn,ifInDiscards,MBytesOut,MBytesIn,ifInErrors
		/*
		num = sprintf(buf,	"%u,%s,%u,%u,%llu,%u,%u,%llu,%u,%u,%u,%u,%u,%u,%llu,%u,%u,%u,%u,%u,%u\n",
				(uint32_t)s->timestamp,
				a,
				s->counter_generic_if_index,
				s->counter_generic_if_type,
				s->counter_generic_if_speed,
				s->counter_generic_if_direction,
				s->counter_generic_if_if_status,
				s->counter_generic_if_in_octets,
				s->counter_generic_if_in_ucast_pkts,
				s->counter_generic_if_in_mcast_pkts,
				s->counter_generic_if_in_bcast_pkts,
				s->counter_generic_if_in_discards,
				s->counter_generic_if_in_errors,
				s->counter_generic_if_in_unknown_proto,
				s->counter_generic_if_out_octets,
				s->counter_generic_if_out_ucast_pkts,
				s->counter_generic_if_out_mcast_pkts,
				s->counter_generic_if_out_bcast_pkts,
				s->counter_generic_if_out_discards,
				s->counter_generic_if_out_errors,
				s->counter_generic_if_promisc
		);
		*/
		write(storage_csv_fd, buf, num);
		node = node->next;
	}
}

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
	logmsg(LOGLEVEL_DEBUG, "Registering csv module");
	storage_modules_register(&storage_mod_csv);
}
