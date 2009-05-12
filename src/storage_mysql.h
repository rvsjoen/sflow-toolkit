#ifndef __storage_mysql_h__
#define __storage_mysql_h__

#include <stdint.h>
#include "logger.h"
#include "util.h"
#include "dataparser.h"
#include "sflowparser.h"

void storage_mysql_load();
void storage_mysql_error();
void storage_mysql_init();
void storage_mysql_destroy();

// Functions to create new tables on demand
void storage_mysql_create_conv_ethernet(uint32_t timestamp);
void storage_mysql_create_conv_ip(uint32_t timestamp);
void storage_mysql_create_conv_tcp(uint32_t timestamp);
void storage_mysql_create_conv_udp(uint32_t timestamp);
void storage_mysql_create_counters(uint32_t timestamp);

// Functions to store different kinds of conversations
void storage_mysql_store_cntr(counter_list_t* list, uint32_t timestamp);
void storage_mysql_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_mysql_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_mysql_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_mysql_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);

#endif
