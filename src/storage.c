#include "storage.h"

MYSQL* db;

void storage_error()
{
		logmsg(LOGLEVEL_ERROR, "storage: %s", mysql_error(db));
		exit_on_error();
}

void storage_init()
{
	if(!mysql_init(db))
		storage_error();
	if(!mysql_real_connect(db, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0))
		storage_error();
}

void storage_destroy()
{
	mysql_close(db);
}

void storage_store_conv_ethernet(conv_key_ethernet_t* key, conv_ethernet_t* conv)
{
	
}

void storage_store_conv_ip(conv_key_ip_t* key, conv_ip_t* conv)
{

}

void storage_store_conv_tcp(conv_key_tcp_t* key, conv_tcp_t* conv)
{

}

void storage_store_conv_udp(conv_key_udp_t* key, conv_udp_t* conv)
{

}
