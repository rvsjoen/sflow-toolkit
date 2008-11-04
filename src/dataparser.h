#ifndef __dataparser_h__
#define __dataparser_h__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "sflowparser.h"
#include "util.h"

typedef struct _conv_key_ethernet {
	uint8_t src[6];  // struct ether_addr
	uint8_t dst[6];
} conv_key_ethernet_t;

typedef struct _conv_key_ip {
	uint32_t src;
	uint32_t dst;
} conv_key_ip_t;

typedef struct _conv_key_udp {
	uint32_t src;
	uint32_t dst;
	uint16_t src_port;
	uint16_t dst_port;
} conv_key_udp_t;

typedef struct _conv_key_tcp {
	uint32_t src;
	uint32_t dst;
	uint16_t src_port;
	uint16_t dst_port;
} conv_key_tcp_t;

typedef union _conv_key_t {
	conv_key_ethernet_t key_ethernet;
	conv_key_ip_t key_ip;
	conv_key_udp_t key_udp;
	conv_key_tcp_t key_tcp;
} conv_key_t;

typedef struct _conv_ethernet {
	conv_key_ethernet_t key;
	uint32_t f_rx;
	uint32_t f_tx;
	uint32_t b_rx;
	uint32_t b_tx;
	uint32_t etype_arp;
	uint32_t etype_rarp;
	uint32_t etype_ip;
	uint32_t etype_pup;
} conv_ethernet_t;

typedef struct _conv_ip {
	conv_key_ip_t key;
	uint32_t f_rx;
	uint32_t f_tx;
	uint32_t b_rx;
	uint32_t b_tx;
	uint32_t protocols[sizeof(uint8_t)];
} conv_ip_t;

typedef struct _conv_tcp {
	conv_key_tcp_t key;
	uint32_t f_rx;
	uint32_t f_tx;
	uint32_t b_rx;
	uint32_t b_tx;
	uint8_t flags;
} conv_tcp_t;

typedef struct _conv_udp {
	conv_key_udp_t key;
	uint32_t f_rx;
	uint32_t f_tx;
	uint32_t b_rx;
	uint32_t b_tx;
	uint8_t flags;
} conv_udp_t;

typedef union _conv {
	conv_ethernet_t conv_ethernet;
	conv_ip_t conv_ip;
	conv_tcp_t conv_tcp;
	conv_udp_t conv_udp;
} conv_t;

typedef struct _conv_list_node {
	conv_t conv;
	struct _conv_list_node* next;
} conv_list_node_t;

typedef struct _conv_list {
	uint32_t num;
	conv_list_node_t* data;
} conv_list_t;

bool is_ip(const uint8_t* pkt);
bool is_tcp(const uint8_t* pkt);
bool is_udp(const uint8_t* pkt);

// Each function assumes the passed pointer is at the start of its respective header
void get_key_ethernet(const uint8_t* pkt, conv_key_ethernet_t* k);
void get_key_ip(const uint8_t* pkt, conv_key_ip_t* k);
void get_key_udp(const uint8_t* pkt, conv_key_udp_t* k);
void get_key_tcp(const uint8_t* pkt, conv_key_tcp_t* k);

// Process a single sample
void process_sample_flow(SFFlowSample* s, conv_list_t* c_ethernet, conv_list_t* c_ip, conv_list_t* c_tcp, conv_list_t* c_udp);
void process_sample_cntr(SFCntrSample* s);

// These are the main entry points for processing a single binary 
// buffer containing samples
void process_file_flow(const char* filename, uint32_t agent);
void process_file_cntr(const char* filename, uint32_t agent);

#endif
