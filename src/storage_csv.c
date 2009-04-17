#include "storage.h"
#include "storage_csv.h"

time_t storage_csv_time;
uint32_t storage_csv_fd = 0;

void storage_csv_store_cntr(counter_list_t* list, uint32_t timestamp){
	storage_csv_fd = shm_open("foobar", O_RDWR | O_CREAT, S_IRWXU);
	char buf[];
	uint32_t num;
	num = sprintf(buf, ";;;;;;\n");
	write(storage_csv_fd, buf, num);
	close(storage_csv_fd);
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
