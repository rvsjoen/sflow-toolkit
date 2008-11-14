#ifndef __storage_h__
#define __storage_h__

#include "logger.h"
#include "util.h"
#include "dataparser.h"
#include <mysql/mysql.h>

#define DB_HOST "localhost"
#define DB_USER	"sflow"
#define DB_PASS	"sflow"
#define DB_NAME "sflow"

// Handle to the database
void storage_error();
void storage_init();
void storage_destroy();

// Functions to store different kinds of conversations
void storage_store_conv_ethernet(conv_key_ethernet_t* key, conv_ethernet_t* conv);
void storage_store_conv_ip(conv_key_ip_t* key, conv_ip_t* conv);
void storage_store_conv_tcp(conv_key_tcp_t* key, conv_tcp_t* conv);
void storage_store_conv_udp(conv_key_udp_t* key, conv_udp_t* conv);

#endif
