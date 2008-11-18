#include "dataparser.h"
#include "storage.h"

conv_list_t* hash_ethernet[HASH_RANGE];
conv_list_t* hash_ip[HASH_RANGE];
conv_list_t* hash_tcp[HASH_RANGE];
conv_list_t* hash_udp[HASH_RANGE];

uint32_t cnt_ethernet;
uint32_t cnt_ip;
uint32_t cnt_tcp;
uint32_t cnt_udp;

void process_file_flow(const char* filename, uint32_t agent, uint32_t timestamp){

	UNUSED_ARGUMENT(agent);
	
	// Zero the hash tables before processing the file
	memset(hash_ethernet, 	0, sizeof(conv_list_t*)*HASH_RANGE);
	memset(hash_ip, 		0, sizeof(conv_list_t*)*HASH_RANGE);
	memset(hash_tcp, 		0, sizeof(conv_list_t*)*HASH_RANGE);
	memset(hash_udp, 		0, sizeof(conv_list_t*)*HASH_RANGE);

	FILE* fd = NULL;
	if((fd = fopen(filename, "r"))){
		SFFlowSample s;
		while(fread(&s, sizeof(SFFlowSample), 1, fd)){
			process_sample_flow(&s);
		}
		fclose(fd);
	} else {
		logmsg(LOGLEVEL_ERROR, "%s", strerror(errno));
	}

	cnt_ethernet = cnt_ip = cnt_tcp = cnt_udp = 0;
	conv_store_ethernet(agent, timestamp);
	conv_store_ip(agent, timestamp);
	conv_store_tcp(agent, timestamp);
	conv_store_udp(agent, timestamp);
}

void process_sample_flow(SFFlowSample* s){
	
	// Declare some keys
	conv_key_ethernet_t key_ethernet;
	conv_key_ip_t 		key_ip;
	conv_key_udp_t 		key_udp;
	conv_key_tcp_t 		key_tcp;
	memset(&key_ethernet, 	0, sizeof(conv_key_ethernet_t));
	memset(&key_ip, 		0, sizeof(conv_key_ip_t));
	memset(&key_tcp, 		0, sizeof(conv_key_tcp_t));
	memset(&key_udp, 		0, sizeof(conv_key_udp_t));

	uint8_t* pkt = s->raw_header;
	
	// Populate the keys from the sample
	get_key_ethernet(s, &key_ethernet);
	conv_list_add(pkt, (conv_key_t*) &key_ethernet, CONV_ETHERNET, s);
	if(is_ip(pkt)){
		get_key_ip(s, &key_ip);
		conv_list_add(pkt, (conv_key_t*) &key_ip, CONV_IP, s);
		if(is_tcp(pkt)){
			get_key_tcp(s, &key_tcp);
			conv_list_add(pkt, (conv_key_t*) &key_tcp, CONV_TCP, s);
		} else if(is_udp(pkt)){
			get_key_udp(s, &key_udp);
			conv_list_add(pkt, (conv_key_t*) &key_udp, CONV_UDP, s);
		}
	}
}

void get_key_ethernet(SFFlowSample* s, conv_key_ethernet_t* k){
	uint8_t* pkt = s->raw_header;

	struct ether_header* hdr = (struct ether_header*) pkt;

	memcpy(k->src, hdr->ether_shost, ETH_ALEN);
	memcpy(k->dst, hdr->ether_dhost, ETH_ALEN);

	k->sflow_input_if = s->sample_input_if_value;
	k->sflow_output_if = s->sample_output_if_value;

}

uint8_t* strip_vlan(const uint8_t* pkt){
	return (uint8_t*) pkt + (4*sizeof(uint8_t));
}

uint8_t* strip_ethernet(const uint8_t* pkt){
	struct ether_header* hdr = (struct ether_header*) pkt;
	uint8_t* p = (uint8_t*) pkt + sizeof(struct ether_header);
	if(ntohs(hdr->ether_type) == ETHERTYPE_VLAN){
		p = strip_vlan(p); 
	}
	return p;
}

void get_key_ip(SFFlowSample* s, conv_key_ip_t* k){
	uint8_t* pkt = s->raw_header;
	uint8_t* p = strip_ethernet(pkt);
	struct iphdr* hdr = (struct iphdr*) p;
	k->src = ntohl(hdr->saddr);
	k->dst = ntohl(hdr->daddr);
	k->sflow_input_if = s->sample_input_if_value;
	k->sflow_output_if = s->sample_output_if_value;
}

uint8_t* strip_ip(const uint8_t* pkt){
	uint8_t* p = (uint8_t*) pkt;
	struct iphdr* hdr = (struct iphdr*) p;
	p += hdr->ihl*4*sizeof(uint8_t);
	return p;
}

void get_key_udp(SFFlowSample* s, conv_key_udp_t* k){
	uint8_t* pkt = s->raw_header;
	uint8_t* p = strip_ethernet(pkt);
	struct iphdr* ip_hdr = (struct iphdr*) p;
	k->src = ntohl(ip_hdr->saddr);
	k->dst = ntohl(ip_hdr->daddr);
	p = strip_ip(p);
	struct udphdr* udp_hdr = (struct udphdr*) p;
	k->src_port = ntohs(udp_hdr->source);
	k->dst_port = ntohs(udp_hdr->dest);
	k->sflow_input_if = s->sample_input_if_value;
	k->sflow_output_if = s->sample_output_if_value;
}

void get_key_tcp(SFFlowSample* s, conv_key_tcp_t* k){
	uint8_t* pkt = s->raw_header;
	uint8_t* p = strip_ethernet(pkt);
	struct iphdr* ip_hdr = (struct iphdr*) p;
	k->src = ntohl(ip_hdr->saddr);
	k->dst = ntohl(ip_hdr->daddr);
	p = strip_ip(p);
	struct tcphdr* tcp_hdr = (struct tcphdr*) p;
	k->src_port = ntohs(tcp_hdr->source);
	k->dst_port = ntohs(tcp_hdr->dest);
	k->sflow_input_if = s->sample_input_if_value;
	k->sflow_output_if = s->sample_output_if_value;
}

bool is_ip(const uint8_t* pkt){
	struct ether_header* hdr = (struct ether_header*) pkt;
	if(ntohs(hdr->ether_type) == ETHERTYPE_IP)
		return true;
	else if(ntohs(hdr->ether_type) == ETHERTYPE_VLAN){
		hdr = (struct ether_header*) strip_vlan(pkt);
		if(ntohs(hdr->ether_type) == ETHERTYPE_IP)
			return true;
	}
	return false;
}

bool is_udp(const uint8_t* pkt){
	struct iphdr* hdr = (struct iphdr*) strip_ethernet(pkt);
	if(is_ip(pkt)){
		if(hdr->protocol == 0x17) // 0x17 UDP
			return true;
	}
	return false;
}

bool is_tcp(const uint8_t* pkt){
	struct iphdr* hdr = (struct iphdr*) strip_ethernet(pkt);
	if(is_ip(pkt)){
		if(hdr->protocol == 0x06) // 0x06 TCP
			return true;
	}
	return false;
}

void conv_update_ethernet(conv_ethernet_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->frames++;
	c->bytes += s->raw_header_frame_length;
	UNUSED_ARGUMENT(pkt);
	UNUSED_ARGUMENT(s);
}

void conv_update_ip(conv_ip_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->packets++;
	UNUSED_ARGUMENT(pkt);
	UNUSED_ARGUMENT(s);
}

void conv_update_tcp(conv_tcp_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->segments++;
	UNUSED_ARGUMENT(pkt);
	UNUSED_ARGUMENT(s);
}

void conv_update_udp(conv_udp_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->segments++;
	UNUSED_ARGUMENT(pkt);
	UNUSED_ARGUMENT(s);
}

conv_t* conv_list_search(conv_list_t* list, conv_key_t* key){
	conv_list_node_t* tmp = list->data;
	while(tmp){
		if(memcmp(key, tmp->key, sizeof(conv_key_t)) == 0){
			return tmp->conv;
		} 
		tmp = tmp->next;
	}
	return NULL;
}

void conv_store_ethernet(uint32_t agent, uint32_t timestamp){
	int i;
	for(i=0; i<HASH_RANGE;i++)
	{
		conv_list_t* list = hash_ethernet[i];

		if(list == NULL)
			continue;

		conv_list_node_t* n = list->data;
		while(n){
			conv_key_ethernet_t* k = (conv_key_ethernet_t*) n->key;
			conv_ethernet_t* c = (conv_ethernet_t*) n->conv;
			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;
			storage_store_conv_ethernet(k, c, agent, timestamp);
			free(k);
			free(c);
			free(tmp);
			cnt_ethernet++;
		}
		free(list);
	}
	logmsg(LOGLEVEL_DEBUG, "Stored %u ethernet conversations", cnt_ethernet);
}

void conv_store_ip(uint32_t agent, uint32_t timestamp){
	int i;
	for(i=0; i<HASH_RANGE;i++)
	{
		conv_list_t* list = hash_ip[i];

		if(list == NULL)
			continue;

		conv_list_node_t* n = list->data;
		while(n){
			conv_key_ip_t* k = (conv_key_ip_t*) n->key;
			conv_ip_t* c = (conv_ip_t*) n->conv;
			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;
			storage_store_conv_ip(k, c, agent, timestamp);
			free(k);
			free(c);
			free(tmp);

			cnt_ip++;
		}
		free(list);
	}
	logmsg(LOGLEVEL_DEBUG, "Stored %u ip conversations", cnt_ip);
}

void conv_store_tcp(uint32_t agent, uint32_t timestamp){
	int i;
	for(i=0; i<HASH_RANGE;i++)
	{
		conv_list_t* list = hash_tcp[i];

		if(list == NULL)
			continue;

		conv_list_node_t* n = list->data;
		while(n){
			conv_key_tcp_t* k = (conv_key_tcp_t*) n->key;
			conv_tcp_t* c = (conv_tcp_t*) n->conv;
			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;
			storage_store_conv_tcp(k, c, agent, timestamp);
			free(k);
			free(c);
			free(tmp);

			cnt_tcp++;
		}
		free(list);
	}
	logmsg(LOGLEVEL_DEBUG, "Stored %u tcp conversations", cnt_tcp);
}

void conv_store_udp(uint32_t agent, uint32_t timestamp){
	int i;
	for(i=0; i<HASH_RANGE;i++)
	{
		conv_list_t* list = hash_udp[i];

		if(list == NULL)
			continue;

		conv_list_node_t* n = list->data;
		while(n){
			conv_key_udp_t* k = (conv_key_udp_t*) n->key;
			conv_udp_t* c = (conv_udp_t*) n->conv;
			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;
			storage_store_conv_udp(k, c, agent, timestamp);
			free(k);
			free(c);
			free(tmp);

			cnt_udp++;
		}
		free(list);
	}
	logmsg(LOGLEVEL_DEBUG, "Stored %u udp conversations", cnt_udp);
}

void conv_list_add(const uint8_t* pkt, conv_key_t* key, uint32_t ctype, SFFlowSample* s){

	int h = 0;
	conv_list_t* list = NULL;
	switch(ctype){
		case CONV_ETHERNET:
			h = hash_key_ethernet((conv_key_ethernet_t*) key);
			list = hash_ethernet[h];
			// First get a pointer to the list we are going to do a linear search on
			if(list == NULL){
				list = (conv_list_t*) malloc(sizeof(conv_list_t));
				memset(list, 0, sizeof(conv_list_t));
				hash_ethernet[h] = list;
			}
			break;
		case CONV_IP:
			h = hash_key_ip((conv_key_ip_t*) key);
			list = hash_ip[h];
			// First get a pointer to the list we are going to do a linear search on
			if(list == NULL){
				list = (conv_list_t*) malloc(sizeof(conv_list_t));
				memset(list, 0, sizeof(conv_list_t));
				hash_ip[h] = list;
			}
			break;
		case CONV_TCP:
			h = hash_key_tcp((conv_key_tcp_t*) key);
			list = hash_tcp[h];
			// First get a pointer to the list we are going to do a linear search on
			if(list == NULL){
				list = (conv_list_t*) malloc(sizeof(conv_list_t));
				memset(list, 0, sizeof(conv_list_t));
				hash_tcp[h] = list;
			}
			break;
		case CONV_UDP:
			h = hash_key_udp((conv_key_udp_t*) key);
			list = hash_udp[h];
			// First get a pointer to the list we are going to do a linear search on
			if(list == NULL){
				list = (conv_list_t*) malloc(sizeof(conv_list_t));
				memset(list, 0, sizeof(conv_list_t));
				hash_udp[h] = list;
			}
			break;
	}


	conv_t* c = NULL;
	c = conv_list_search(list, key);

	if(c == NULL){
		// Allocate a conversation
		c = (conv_t*) malloc(sizeof(conv_t));
		memset(c, 0, sizeof(conv_t));

		// Allocate a list node
		conv_list_node_t* n = (conv_list_node_t*) malloc(sizeof(conv_list_node_t));
		memset(n, 0, sizeof(conv_list_node_t));

		conv_key_t* k = (conv_key_t*) malloc(sizeof(conv_key_t));
		memset(k, 0, sizeof(conv_key_t));
		memcpy(k, key, sizeof(conv_key_t));

		n->conv = c;
		n->key = k;

		if(list->num==0){
			list->data = n;
		} else {
			n->next = list->data;
			list->data = n;
		}

		list->num++;
	} 

	// We have a valid reference to a conversation, update the information
	// with the values from the packet
	switch(ctype){
		case CONV_ETHERNET:
			conv_update_ethernet(&(c->conv_ethernet), pkt, s);
			break;
		case CONV_IP:
			conv_update_ip(&(c->conv_ip), pkt, s);
			break;
		case CONV_TCP:
			conv_update_tcp(&(c->conv_tcp), pkt, s);
			break;
		case CONV_UDP:
			conv_update_udp(&(c->conv_udp), pkt, s);
			break;
	}
}

int hash_key_ethernet(conv_key_ethernet_t* k){
	int result = 0;
	result += k->sflow_input_if % HASH_RANGE;
	result += k->sflow_output_if % HASH_RANGE;
	int i;
	for(i=0; i<6; i++){
		result += k->src[i] % HASH_RANGE;
		result += k->dst[i] % HASH_RANGE;
	}
	return result % HASH_RANGE;
}

int hash_key_ip(conv_key_ip_t* k){
	int result = 0;
	result += k->sflow_input_if % HASH_RANGE;
	result += k->sflow_output_if % HASH_RANGE;
	result += k->src % HASH_RANGE;
	result += k->dst % HASH_RANGE;
	return result % HASH_RANGE;
}

int hash_key_tcp(conv_key_tcp_t* k){
	int result = 0;
	result += k->sflow_input_if % HASH_RANGE;
	result += k->sflow_output_if % HASH_RANGE;
	result += k->src % HASH_RANGE;
	result += k->dst % HASH_RANGE;
	result += k->src_port % HASH_RANGE;
	result += k->dst_port % HASH_RANGE;
	return result % HASH_RANGE;
}

int hash_key_udp(conv_key_udp_t* k){
	int result = 0;
	result += k->sflow_input_if % HASH_RANGE;
	result += k->sflow_output_if % HASH_RANGE;
	result += k->src % HASH_RANGE;
	result += k->dst % HASH_RANGE;
	result += k->src_port % HASH_RANGE;
	result += k->dst_port % HASH_RANGE;
	return result % HASH_RANGE;
}
