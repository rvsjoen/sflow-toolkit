#include "storage.h"

storage_module_list_t* storage_modules;

void storage_init(){
	logmsg(LOGLEVEL_DEBUG, "Generic init");
}

void storage_destroy(){
	logmsg(LOGLEVEL_DEBUG, "Generic destroy");
}

void storage_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	logmsg(LOGLEVEL_DEBUG, "Generic store ethernet");
}

void storage_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	logmsg(LOGLEVEL_DEBUG, "Generic store ip");
}

void storage_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	logmsg(LOGLEVEL_DEBUG, "Generic store tcp");
}

void storage_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	logmsg(LOGLEVEL_DEBUG, "Generic store udp");
}

void storage_store_cntr(counter_list_t* list){
	logmsg(LOGLEVEL_DEBUG, "Generic store counters");
}

void storage_system_init(){
	storage_modules = (storage_module_list_t*) malloc(sizeof(storage_module_list_t));
	memset(storage_modules, 0, sizeof(storage_module_list_t));
}

void storage_system_destroy(){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_list_node_t* tmp = node;
		node = node->next;
		free(tmp);
	}
	free(storage_modules);
}

void storage_modules_register(storage_module_t* module){
	storage_module_list_node_t* node =  (storage_module_list_node_t*) malloc(sizeof(storage_module_list_node_t));
	memset(node, 0, sizeof(storage_module_list_node_t));
	if (storage_modules->data == NULL)
		storage_modules->data = node;
	else {
		node->next = storage_modules->data;
		storage_modules->data = node;
	}
	node->module = module;
	storage_modules->num++;
}

void storage_modules_init(){
	logmsg(LOGLEVEL_INFO, "Initializing storage modules:");
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		logmsg(LOGLEVEL_INFO, "... %s", module->name);
		if(module->init == NULL)
			storage_init();
		else
			module->init();
		node = node->next;
	}
}

void storage_modules_destroy(){
	logmsg(LOGLEVEL_INFO, "Destroying storage modules:");
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		logmsg(LOGLEVEL_INFO, "... %s", module->name);
		if(module->destroy == NULL)
			storage_destroy();
		else
			module->destroy();
		node = node->next;
	}
}

void storage_modules_store_cntr(counter_list_t* list){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		if(module->store_cntr == NULL)
			storage_store_cntr(list);
		else
			module->store_cntr(list);
		node = node->next;
	}
}

void storage_modules_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		if(module->store_conv_ethernet == NULL)
			storage_store_conv_ethernet(list, num, agent, timestamp);
		else
			module->store_conv_ethernet(list, num, agent, timestamp);
		node = node->next;
	}
}

void storage_modules_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		if(module->store_conv_ip == NULL)
			storage_store_conv_ip(list, num, agent, timestamp);
		else
			module->store_conv_ip(list, num, agent, timestamp);
		node = node->next;
	}
}

void storage_modules_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		if(module->store_conv_tcp == NULL)
			storage_store_conv_tcp(list, num, agent, timestamp);
		else
			module->store_conv_tcp(list, num, agent, timestamp);
		node = node->next;
	}
}

void storage_modules_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	storage_module_list_node_t* node;
	node = storage_modules->data;
	while(node){
		storage_module_t* module = node->module;
		if(module->store_conv_udp == NULL)
			storage_store_conv_udp(list, num, agent, timestamp);
		else
			module->store_conv_udp(list, num, agent, timestamp);
		node = node->next;
	}
}
