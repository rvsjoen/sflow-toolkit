#include "storage.h"

MYSQL db;

#define BULK_INSERT_NUM  500
#define BULK_INSERT_SIZE 150

void storage_error(){
		logmsg(LOGLEVEL_ERROR, "storage: %s", mysql_error(&db));
		exit_on_error();
}

void storage_init(){
	if(!mysql_init(&db))
		storage_error();
	if(!mysql_real_connect(&db, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0))
		storage_error();
	logmsg(LOGLEVEL_DEBUG, "storage: connected to database");
}

void storage_destroy(){
	mysql_close(&db);
	logmsg(LOGLEVEL_DEBUG, "storage: closed database connection");
}

void storage_store_conv_ethernet_list(conv_list_t** hash_ethernet, uint32_t num, uint32_t agent, uint32_t timestamp){

	char* query;
	char* ptr;
	query = (char*) malloc(sizeof(char)*BULK_INSERT_NUM*BULK_INSERT_SIZE);

	char stmt[] = "INSERT INTO conv_ethernet (timestamp,agent,input_if,output_if,src,dst,bytes,frames) VALUES ";

	ptr = query;
	memset(query, 0, sizeof(char)*BULK_INSERT_NUM*BULK_INSERT_SIZE); 
	ptr += sizeof(char) * sprintf(ptr, stmt);

	uint32_t cnt_ethernet = 0;

	uint32_t i;
	for(i=0; i<num;i++)
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

			char a[16];
			char src[18];
			char dst[18];
			strncpy(src, ether_ntoa((const struct ether_addr *)k->src), 18);
			strncpy(dst, ether_ntoa((const struct ether_addr *)k->dst), 18);
			num_to_ip(agent, a);

			ptr += sizeof(char) * sprintf(ptr, "(%u, '%s', %u, %u, '%s', '%s', %u, %u)",
				timestamp,
				a,
				k->sflow_input_if,
				k->sflow_output_if,
				src,
				dst,
				c->bytes, 
				c->frames
		   	);

			free(k);
			free(c);
			free(tmp);
			cnt_ethernet++;

			if(i%BULK_INSERT_NUM == 0){
				logmsg(LOGLEVEL_DEBUG, "%s", query);
				mysql_query(&db, query);
				ptr = query;
				memset(query, 0, sizeof(char)*BULK_INSERT_NUM*BULK_INSERT_SIZE); 
				ptr += sizeof(char) * sprintf(ptr, stmt);
			}

		}
		free(list);
	}

	logmsg(LOGLEVEL_DEBUG, "%s", query);
	mysql_query(&db, query);
	free(query);
	logmsg(LOGLEVEL_DEBUG, "Stored %u ethernet conversations", cnt_ethernet);
}

void storage_store_conv_ethernet(conv_key_ethernet_t* key, conv_ethernet_t* conv, uint32_t agent, uint32_t timestamp){
	// Remember sflow_input_if and sflow_output_if
	char* query;
	char a[16];
	char src[18];
	char dst[18];
	strncpy(src, ether_ntoa((const struct ether_addr *)key->src), 18);
	strncpy(dst, ether_ntoa((const struct ether_addr *)key->dst), 18);

	num_to_ip(agent, a);
	asprintf(&query, "INSERT INTO conv_ethernet (timestamp,agent,input_if,output_if,src,dst,bytes,frames) VALUES (%u, '%s', %u, %u, '%s', '%s', %u, %u)", 
			timestamp,
			a,
			key->sflow_input_if,
			key->sflow_output_if,
			src,
			dst,
			conv->bytes, 
			conv->frames
			);
	mysql_query(&db, query);
//	logmsg(LOGLEVEL_DEBUG, "%s", query);
	free(query);
}

void storage_store_conv_ip(conv_key_ip_t* key, conv_ip_t* conv, uint32_t agent, uint32_t timestamp){
	char* query;
	char src[16];
	char dst[16];
	char a[16];
	num_to_ip(agent, a);
	num_to_ip(key->src, src);
	num_to_ip(key->dst, dst);
	asprintf(&query, "INSERT INTO conv_ip (timestamp,agent,input_if,output_if,src,dst,bytes,frames) VALUES (%u, '%s', %u, %u, '%s', '%s', %u, %u)", 
			timestamp,
			a,
			key->sflow_input_if,
			key->sflow_output_if,
			src,
			dst,
			conv->bytes,
			conv->frames
			);
	mysql_query(&db, query);
//	logmsg(LOGLEVEL_DEBUG, "%s", query);
	free(query);
}

void storage_store_conv_tcp(conv_key_tcp_t* key, conv_tcp_t* conv, uint32_t agent, uint32_t timestamp){
	char* query;
	char src[16];
	char dst[16];
	char a[16];
	num_to_ip(agent, a);
	num_to_ip(key->src, src);
	num_to_ip(key->dst, dst);
	asprintf(&query, "INSERT INTO conv_tcp (timestamp,agent,input_if,output_if,src,sport,dst,dport,bytes,frames) VALUES (%u, '%s', %u, %u, '%s', %u,'%s',%u, %u, %u)",
			timestamp,
			a,
			key->sflow_input_if,
			key->sflow_output_if,
			src,
			key->src_port,
			dst,
			key->dst_port,
			conv->bytes,
			conv->frames
			);
	mysql_query(&db, query);
//	logmsg(LOGLEVEL_DEBUG, "%s", query);
	free(query);
}

void storage_store_conv_udp(conv_key_udp_t* key, conv_udp_t* conv, uint32_t agent, uint32_t timestamp){
	char* query;
	char src[16];
	char dst[16];
	char a[16];
	num_to_ip(agent, a);
	num_to_ip(key->src, src);
	num_to_ip(key->dst, dst);
	asprintf(&query, "INSERT INTO conv_udp (timestamp,agent,input_if,output_if,src,sport,dst,dport,bytes,frames) VALUES (%u, '%s', %u, %u, '%s', %u,'%s',%u, %u, %u)",
			timestamp,
			a,
			key->sflow_input_if,
			key->sflow_output_if,
			src,
			key->src_port,
			dst,
			key->dst_port,
			conv->bytes,
			conv->frames
			);
	mysql_query(&db, query);
//	logmsg(LOGLEVEL_DEBUG, "%s", query);
	free(query);
}

void storage_store_cntr(SFCntrSample* s){
	char* query;
	char a[16];
	num_to_ip(s->agent_address, a);

	asprintf(&query, 
			"INSERT INTO counters (\
			timestamp,agent,if_index,if_type,if_speed,if_direction,if_if_status,\
			if_in_octets,if_in_ucast_pkts,if_in_mcast_pkts,if_in_bcast_pkts,\
			if_in_discards,if_in_errors,if_in_unknown_proto,if_out_octets,\
			if_out_ucast_pkts,if_out_mcast_pkts,if_out_bcast_pkts,if_out_discards,\
			if_out_errors,if_promisc)\
			VALUES (%u, '%s', %u, %u, %llu, %u, %u, %llu, %u, %u, %u, %u, %u, %u, \
					%llu, %u, %u, %u, %u, %u, %u)", 
			(uint32_t)s->timestamp,
			a,
			s->counter_generic_if_index,
			s->counter_generic_if_type,
			s->counter_generic_if_speed,
			s->counter_generic_if_direction,
			s->counter_generic_if_if_status,
			s->counter_generic_if_in_octets,
			s->counter_generic_if_in_ucast_pkts,
			s->counter_generic_if_in_mcast_pkts,
			s->counter_generic_if_in_bcast_pkts,
			s->counter_generic_if_in_discards,
			s->counter_generic_if_in_errors,
			s->counter_generic_if_in_unknown_proto,
			s->counter_generic_if_out_octets,
			s->counter_generic_if_out_ucast_pkts,
			s->counter_generic_if_out_mcast_pkts,
			s->counter_generic_if_out_bcast_pkts,
			s->counter_generic_if_out_discards,
			s->counter_generic_if_out_errors,
			s->counter_generic_if_promisc
			);
	mysql_query(&db, query);
//	logmsg(LOGLEVEL_DEBUG, "%s", query);
	free(query);
}
