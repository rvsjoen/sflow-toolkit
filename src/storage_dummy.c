#include "storage.h"
#include "storage_dummy.h"

storage_module_t storage_mod_dummy = {
	.name 					= "dummy",
	.init 					= NULL,
	.destroy 				= NULL,
	.store_cntr 			= NULL,
	.store_conv_ethernet 	= NULL,
	.store_conv_ip 			= NULL,
	.store_conv_tcp 		= NULL,
	.store_conv_udp 		= NULL
};

void storage_dummy_load(){
	logmsg(LOGLEVEL_DEBUG, "Registering %s module", storage_mod_dummy.name);
	storage_modules_register(&storage_mod_dummy);
}
