#include "dataparser.h"

void process_file_flow(const char* filename, uint32_t agent){

	UNUSED_ARGUMENT(agent);

	// Allocate some lists and zero them
	conv_list_t c_ethernet, c_ip, c_tcp, c_udp;
	memset(&c_ethernet, 0, sizeof(conv_list_t));
	memset(&c_ip, 0, sizeof(conv_list_t));
	memset(&c_tcp, 0, sizeof(conv_list_t));
	memset(&c_udp, 0, sizeof(conv_list_t));
	
	FILE* fd = NULL;
	if((fd = fopen(filename, "r"))){
		SFFlowSample s;
		while(fread(&s, sizeof(SFFlowSample), 1, fd)){
			process_sample_flow(&s, &c_ethernet, &c_ip, &c_tcp, &c_udp);
		}
		fclose(fd);
	} else {
		logmsg(LOGLEVEL_ERROR, "%s", strerror(errno));
	}

	conv_print_ethernet(&c_ethernet);
	conv_print_ip(&c_ip);
	conv_print_tcp(&c_tcp);
	conv_print_udp(&c_udp);
}

void process_sample_flow(SFFlowSample* s, conv_list_t* c_ethernet, conv_list_t* c_ip, conv_list_t* c_tcp, conv_list_t* c_udp){
	
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
	conv_list_add(c_ethernet, pkt, (conv_key_t*) &key_ethernet, CONV_ETHERNET, s);
	if(is_ip(pkt)){
		get_key_ip(s, &key_ip);
		conv_list_add(c_ip, pkt, (conv_key_t*) &key_ip, CONV_IP, s);
		if(is_tcp(pkt)){
			get_key_tcp(s, &key_tcp);
			conv_list_add(c_tcp, pkt, (conv_key_t*) &key_tcp, CONV_TCP, s);
		} else if(is_udp(pkt)){
			get_key_udp(s, &key_udp);
			conv_list_add(c_udp, pkt, (conv_key_t*) &key_udp, CONV_UDP, s);
		}
	}
}

void get_key_ethernet(SFFlowSample* s, conv_key_ethernet_t* k){
	uint8_t* pkt = s->raw_header;
	struct ether_header* hdr = (struct ether_header*) pkt;
	memcpy(k->src, hdr->ether_shost, ETH_ALEN);
	memcpy(k->dst, hdr->ether_dhost, ETH_ALEN);
//	k->sflow_input_if = s->sample_input_if_value;
//	k->sflow_output_if = s->sample_output_if_value;
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
//	k->sflow_input_if = s->sample_input_if_value;
//	k->sflow_output_if = s->sample_output_if_value;
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
//	k->sflow_input_if = s->sample_input_if_value;
//	k->sflow_output_if = s->sample_output_if_value;
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
//	k->sflow_input_if = s->sample_input_if_value;
//	k->sflow_output_if = s->sample_output_if_value;
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
	c->f_rx++;
	c->b_rx += s->raw_header_frame_length;
}

void conv_update_ip(conv_ip_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->f_rx++;
}

void conv_update_tcp(conv_tcp_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->f_rx++;
}

void conv_update_udp(conv_udp_t* c, const uint8_t* pkt, SFFlowSample* s){
	c->f_rx++;
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

void conv_print_ethernet(conv_list_t* list){
	conv_list_node_t* n = list->data;
	printf("\nEthernet conversation list (%u conversations)\n", list->num);
	while(n){
		conv_key_ethernet_t* k = (conv_key_ethernet_t*) n->key;
		conv_ethernet_t* c = (conv_ethernet_t*) n->conv;
		char msrc[32];
		char mdst[32];
		strncpy(msrc, ether_ntoa((struct ether_addr*)k->src), 32);
		strncpy(mdst, ether_ntoa((struct ether_addr*)k->dst), 32);
		printf("%s -> %s %u %u\n", msrc, mdst, c->f_rx, c->b_rx);
		n = n->next;
	}
}

void conv_print_ip(conv_list_t* list){
	conv_list_node_t* n = list->data;
	printf("\nIP conversation list (%u conversations)\n", list->num);
	while(n){
		n = n->next;
	}
}

void conv_print_tcp(conv_list_t* list){
	conv_list_node_t* n = list->data;
	printf("\nTCP conversation list (%u conversations)\n", list->num);
	while(n){
		n = n->next;
	}
}

void conv_print_udp(conv_list_t* list){
	conv_list_node_t* n = list->data;
	printf("\nUDP conversation list (%u conversations)\n", list->num);
	while(n){
		n = n->next;
	}
}

void conv_list_add(conv_list_t* list, const uint8_t* pkt, conv_key_t* key, uint32_t ctype, SFFlowSample* s){

	UNUSED_ARGUMENT(pkt);

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
