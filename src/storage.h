#ifndef __storage_h__
#define __storage_h__

#include "logger.h"
#include "util.h"
#include "dataparser.h"

typedef struct _storage_module {
	uint8_t name[32];
	void (*init)(void);
	void (*destroy)(void);
	void (*store_cntr)(counter_list_t*);
	void (*store_conv_ethernet)(conv_list_t**, uint32_t, uint32_t, uint32_t);
	void (*store_conv_ip)(conv_list_t**, uint32_t, uint32_t, uint32_t);
	void (*store_conv_tcp)(conv_list_t**, uint32_t, uint32_t, uint32_t);
	void (*store_conv_udp)(conv_list_t**, uint32_t, uint32_t, uint32_t);
} storage_module_t;

typedef struct _storage_module_list_node {
	struct _storage_module_list_node* next;
	storage_module_t* module;
} storage_module_list_node_t;

typedef struct _storage_module_list {
	storage_module_list_node_t* data;
	uint32_t num;
} storage_module_list_t;

// Functions to initialize and destroy the storage system
void storage_system_init();
void storage_system_destroy();

// Functions to manage the modules
void storage_modules_init();
void storage_modules_register(storage_module_t* module);
void storage_modules_destroy();
void storage_modules_store_cntr(counter_list_t* list);
void storage_modules_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_modules_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_modules_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_modules_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);

// Generic functions for the modules, these are invoked if the module does not specify it's own
void storage_init();
void storage_destroy();
void storage_store_cntr(counter_list_t* list);
void storage_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);
void storage_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp);

#endif
