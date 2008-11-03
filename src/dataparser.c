#include "dataparser.h"

void process_file_flow(const char* filename, uint32_t agent){

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

	// Done building the lists, run some queries and dump
	// the results in to the database
}

void process_sample_flow(SFFlowSample* s, conv_list_t* c_ethernet, conv_list_t* c_ip, conv_list_t* c_tcp, conv_list_t* c_udp){
	
	// Declare some keys
	conv_key_ethernet_t key_ethernet;
	conv_key_ip_t 		key_ip;
	conv_key_udp_t 		key_udp;
	conv_key_tcp_t 		key_tcp;

	uint8_t* pkt = s->raw_header;
	
	// Populate the keys from the sample
	get_key_ethernet(pkt, &key_ethernet);

	char msrc[32];
	char mdst[32];
	strncpy(msrc, ether_ntoa(key_ethernet.src), 32);
	strncpy(mdst, ether_ntoa(key_ethernet.dst), 32);

	printf(" (%s  %s) ", msrc, mdst);

	if(is_ip(pkt)){
		get_key_ip(pkt, &key_ip);

		num_to_ip(key_ip.src, msrc);
		num_to_ip(key_ip.dst, mdst);
		printf(" (%s  %s) ", msrc, mdst);

		
		if(is_tcp(pkt)){
			get_key_tcp(pkt, &key_tcp);
			num_to_ip(key_tcp.src, msrc);
			num_to_ip(key_tcp.dst, mdst);
			printf(" (%s:%u  %s:%u) ", msrc, key_tcp.src_port, mdst, key_tcp.dst_port ); 

		} else if(is_udp(pkt)){
			get_key_udp(pkt, &key_udp);
			num_to_ip(key_udp.src, msrc);
			num_to_ip(key_udp.dst, mdst);
			printf(" (%s:%u  %s:%u) ", msrc, key_udp.src_port, mdst, key_udp.dst_port ); 
		}
	}
	printf("\n");
}

void get_key_ethernet(const char* pkt, conv_key_ethernet_t* k){
	struct ether_header* hdr = (struct ether_header*) pkt;
	memcpy(k->src, hdr->ether_shost, ETH_ALEN);
	memcpy(k->dst, hdr->ether_dhost, ETH_ALEN);
	printf("ETHERNET");
}

char* strip_vlan(const char* pkt){
	return pkt + (4*sizeof(uint8_t));
}

char* strip_ethernet(const char* pkt){
	struct ether_header* hdr = (struct ether_header*) pkt;
	char* p = pkt + sizeof(struct ether_header);
	if(ntohs(hdr->ether_type) == ETHERTYPE_VLAN){
		p = strip_vlan(p); 
	}
	return p;
}

void get_key_ip(const char* pkt, conv_key_ip_t* k){
	printf(" IP");
	char* p = strip_ethernet(pkt);
	struct iphdr* hdr = (struct iphdr*) p;
	k->src = ntohl(hdr->saddr);
	k->dst = ntohl(hdr->daddr);
}

char* strip_ip(const uint8_t* pkt){
	uint8_t* p = pkt;
	struct iphdr* hdr = (struct iphdr*) p;
	p += hdr->ihl*4*sizeof(uint8_t);
	return p;
}

void get_key_udp(const char* pkt, conv_key_udp_t* k){
	uint8_t* p = strip_ethernet(pkt);
	struct iphdr* ip_hdr = (struct iphdr*) p;
	k->src = ntohl(ip_hdr->saddr);
	k->dst = ntohl(ip_hdr->daddr);
	p = strip_ip(p);

	struct udphdr* udp_hdr = (struct udphdr*) p;
	k->src_port = ntohs(udp_hdr->source);
	k->dst_port = ntohs(udp_hdr->dest);
	printf("  UDP");
}

void get_key_tcp(const char* pkt, conv_key_tcp_t* k){
	uint8_t* p = strip_ethernet(pkt);

	struct iphdr* ip_hdr = (struct iphdr*) p;
	k->src = ntohl(ip_hdr->saddr);
	k->dst = ntohl(ip_hdr->daddr);
	p = strip_ip(p);

	struct tcphdr* tcp_hdr = (struct tcphdr*) p;
	k->src_port = ntohs(tcp_hdr->source);
	k->dst_port = ntohs(tcp_hdr->dest);
	
	printf("  TCP");
}

bool is_ip(const char* pkt){
	struct ether_header* hdr = (struct ether_header*) pkt;
	if(ntohs(hdr->ether_type) == ETHERTYPE_IP)
		return true;
	else if(ntohs(hdr->ether_type) == ETHERTYPE_VLAN){
		hdr = strip_vlan(pkt);
		if(ntohs(hdr->ether_type) == ETHERTYPE_IP)
			return true;
	}
	return false;
}

bool is_udp(const char* pkt){
	struct iphdr* hdr = (struct iphdr*) strip_ethernet(pkt);
	if(is_ip(pkt)){
		if(hdr->protocol == 0x17) // 0x17 UDP
			return true;
	}
	return false;
}

bool is_tcp(const char* pkt){
	struct iphdr* hdr = (struct iphdr*) strip_ethernet(pkt);
	if(is_ip(pkt)){
		if(hdr->protocol == 0x06) // 0x06 TCP
			return true;
	}
	return false;
}
