#include "storage.h"
#include "storage_rrd.h"

void storage_rrd_store_cntr(counter_list_t* list){

}

storage_module_t storage_mod_rrd = {
	.name 					= "rrd",
	.init 					= NULL,
	.destroy 				= NULL,
	.store_cntr 			= storage_rrd_store_cntr,
	.store_conv_ethernet 	= NULL,
	.store_conv_ip 			= NULL,
	.store_conv_tcp 		= NULL,
	.store_conv_udp 		= NULL
};

void storage_rrd_load(){
	logmsg(LOGLEVEL_DEBUG, "Registering rrd module");
	storage_modules_register(&storage_mod_rrd);
}
