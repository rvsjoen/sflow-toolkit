#ifndef __storage_h__
#define __storage_h__

#include <stdint.h>
#include "logger.h"
#include "util.h"
#include "dataparser.h"
#include "sflowparser.h"
#include <mysql/mysql.h>

#define DB_HOST "localhost"
#define DB_USER	"sflow"
#define DB_PASS	"sflow"
#define DB_NAME "sflow"

void storage_error();
void storage_init();
void storage_destroy();

// Functions to store different kinds of conversations
void storage_store_conv_ethernet(conv_key_ethernet_t*, conv_ethernet_t*, uint32_t agent, uint32_t timestamp);
void storage_store_conv_ip(conv_key_ip_t* key, conv_ip_t* conv, uint32_t agent, uint32_t timestamp);
void storage_store_conv_tcp(conv_key_tcp_t* key, conv_tcp_t* conv, uint32_t agent, uint32_t timestamp);
void storage_store_conv_udp(conv_key_udp_t* key, conv_udp_t* conv, uint32_t agent, uint32_t timestamp);
void storage_store_cntr(SFCntrSample* s);

#endif
