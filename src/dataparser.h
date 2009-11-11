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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sflowparser.h"
#include "util.h"

#define CONV_ETHERNET 0x0
#define CONV_IP 0x1
#define CONV_TCP 0x2
#define CONV_UDP 0x3
#define COUNTERS 0x4

#define HASH_RANGE 		50000

// Ethernet protocol ID's
#define ETHERTYPE_PUP       0x0200      /* Xerox PUP */
#define ETHERTYPE_SPRITE    0x0500      /* Sprite */
#define ETHERTYPE_IP        0x0800      /* IP */
#define ETHERTYPE_ARP       0x0806      /* Address resolution */
#define ETHERTYPE_REVARP    0x8035      /* Reverse ARP */
#define ETHERTYPE_AT        0x809B      /* AppleTalk protocol */
#define ETHERTYPE_AARP      0x80F3      /* AppleTalk ARP */
#define ETHERTYPE_VLAN      0x8100      /* IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_IPX       0x8137      /* IPX */
#define ETHERTYPE_IPV6      0x86dd      /* IP protocol version 6 */
#define ETHERTYPE_LOOPBACK  0x9000      /* used to test interfaces */

// TCP Flags
#define TCP_URG 0x0
#define TCP_ACK 0x1
#define TCP_PSH 0x2
#define TCP_RST 0x3
#define TCP_SYN 0x4
#define TCP_FIN 0x5

typedef struct _conv_key_ethernet {
	uint32_t sflow_input_if;
	uint32_t sflow_output_if;
	uint8_t src[ETH_ALEN];  // struct ether_addr
	uint8_t dst[ETH_ALEN];
} conv_key_ethernet_t;

typedef struct _conv_key_ip {
	uint32_t sflow_input_if;
	uint32_t sflow_output_if;
	uint32_t src;
	uint32_t dst;
} conv_key_ip_t;

typedef struct _conv_key_udp {
	uint32_t sflow_input_if;
	uint32_t sflow_output_if;
	uint32_t src;
	uint32_t dst;
	uint16_t src_port;
	uint16_t dst_port;
} conv_key_udp_t;

typedef struct _conv_key_tcp {
	uint32_t sflow_input_if;
	uint32_t sflow_output_if;
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

typedef struct _ethernet_protocols {
	uint32_t ethertype_ip;
	uint32_t ethertype_arp;
	uint32_t ethertype_rarp;
	uint32_t ethertype_802_1q;
	uint32_t ethertype_ipv6;
} ethernet_protocols;

typedef struct _conv_ethernet {
	uint32_t bytes;
	uint32_t frames;
	ethernet_protocols protocols;
	uint32_t srate;
} conv_ethernet_t;

typedef struct _conv_ip {
	uint32_t frames;
	uint32_t bytes;
	uint32_t version[16];
	uint32_t protocol[256];
	uint32_t srate;
} conv_ip_t;

typedef struct _conv_tcp {
	uint32_t frames;
	uint32_t bytes;
	uint32_t flags[6];
	uint32_t srate;
} conv_tcp_t;

typedef struct _conv_udp {
	uint32_t frames;
	uint32_t bytes;
	uint32_t srate;
} conv_udp_t;

typedef union _conv {
	conv_ethernet_t conv_ethernet;
	conv_ip_t conv_ip;
	conv_tcp_t conv_tcp;
	conv_udp_t conv_udp;
} conv_t;

typedef struct _conv_list_node {
	conv_key_t* key;
	conv_t* conv;
	struct _conv_list_node* next;
} conv_list_node_t;

typedef struct _conv_list {
	uint32_t num;
	conv_list_node_t* data;
} conv_list_t;

typedef struct _counter_list_node {
	SFCntrSample sample;
	struct _counter_list_node* next;
} counter_list_node_t;

typedef struct _counter_list {
	uint32_t num;
	counter_list_node_t* data;
}	counter_list_t;

bool is_ip(const uint8_t* pkt);
bool is_tcp(const uint8_t* pkt);
bool is_udp(const uint8_t* pkt);

int hash_key_ethernet(conv_key_ethernet_t* k);
int hash_key_ip(conv_key_ip_t* k);
int hash_key_tcp(conv_key_tcp_t* k);
int hash_key_udp(conv_key_udp_t* k);

void get_key_ethernet(SFFlowSample* spl, conv_key_ethernet_t* k);
void get_key_ip(SFFlowSample* spl, conv_key_ip_t* k);
void get_key_udp(SFFlowSample* spl, conv_key_udp_t* k);
void get_key_tcp(SFFlowSample* spl, conv_key_tcp_t* k);

void conv_update_ethernet(conv_ethernet_t* c, const uint8_t* pkt, SFFlowSample* s);
void conv_update_ip(conv_ip_t* c, const uint8_t* pkt, SFFlowSample* s);
void conv_update_tcp(conv_tcp_t* c, const uint8_t* pkt, SFFlowSample* s);
void conv_update_udp(conv_udp_t* c, const uint8_t* pkt, SFFlowSample* s);

conv_t* conv_list_search(conv_list_t* list, conv_key_t* key);
void conv_list_add(const uint8_t* pkt, conv_key_t* key, uint32_t ctype, SFFlowSample* s);
void conv_list_free(conv_list_t** list);

// Process a single sample
void process_sample_flow(SFFlowSample* s);
void process_sample_cntr(SFCntrSample* s);

// These are the main entry points for processing a single binary 
// buffer containing samples
void process_file_flow(const char* filename, uint32_t agent, uint32_t timestamp);
void process_file_cntr(const char* filename, uint32_t agent, uint32_t timestamp);

#endif
