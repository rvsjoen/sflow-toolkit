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
void storage_store_cntr(SFCntrSample* s);
void storage_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);

#endif
