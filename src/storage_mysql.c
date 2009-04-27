#include <mysql/mysql.h>
#include "storage.h"
#include "storage_mysql.h"

#define BULK_INSERT_NUM  			500
#define BULK_INSERT_SIZE_ETHERNET 	200
#define BULK_INSERT_SIZE_IP 		200
#define BULK_INSERT_SIZE_TCP 		200
#define BULK_INSERT_SIZE_UDP 		200
#define BULK_INSERT_SIZE_CNTR		600
#define TABLE_INTERVAL				1440 // Minutes TODO Configuration parameter
#define PATH_SHM					"/dev/shm" // TODO Configuration parameter

MYSQL db;

uint32_t table_conv_ethernet;
uint32_t table_conv_ip;
uint32_t table_conv_tcp;
uint32_t table_conv_udp;
uint32_t table_counters;

char table_conv_ethernet_name[32];
char table_conv_ip_name[32];
char table_conv_tcp_name[32];
char table_conv_udp_name[32];
char table_counters_name[32];

void storage_mysql_error(){
		logmsg(LOGLEVEL_ERROR, "storage-mysql: %s", mysql_error(&db));
		exit_on_error();
}

void storage_mysql_init(){
	if(!mysql_init(&db))
		storage_mysql_error();
	if(!mysql_real_connect(&db, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0))
		storage_mysql_error();
	logmsg(LOGLEVEL_DEBUG, "storage-mysql: connected to database");
	storage_mysql_create_conv_ethernet(time(NULL)/60);
	storage_mysql_create_conv_ip(time(NULL)/60);
	storage_mysql_create_conv_tcp(time(NULL)/60);
	storage_mysql_create_conv_udp(time(NULL)/60);
}

void storage_mysql_destroy(){
	mysql_close(&db);
	logmsg(LOGLEVEL_DEBUG, "storage-mysql: closed database connection");
}

void storage_mysql_create_conv_ethernet(uint32_t timestamp){
	char title[32];
	struct tm* tmp;

	time_t t = (time_t) timestamp;
	t *= 60;
	tmp = localtime(&t);

	strftime(title, 32, "conv_ethernet_%d%m%y", tmp);
	char query[512];
	MYSQL_RES* res;
	res = mysql_list_tables(&db, title);
	if ( mysql_num_rows(res) > 0 ){
		logmsg(LOGLEVEL_DEBUG, "table %s exists, doing nothing", title);
	} else {
		logmsg(LOGLEVEL_DEBUG, "table %s does not exist, creating table", title);
		sprintf(query,"CREATE TABLE %s ( timestamp INTEGER UNSIGNED, agent VARCHAR(16), input_if INTEGER UNSIGNED, output_if INTEGER UNSIGNED, src VARCHAR(18), dst VARCHAR(18), bytes INTEGER UNSIGNED, frames INTEGER UNSIGNED, CONSTRAINT %s_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,dst)) ENGINE=myisam", title, title);
		logmsg(LOGLEVEL_DEBUG, "query: %s", query);
		mysql_query(&db, query);
	}
	table_conv_ethernet = timestamp/TABLE_INTERVAL;
	strncpy(table_conv_ethernet_name, title, 32);
}

void storage_mysql_create_conv_ip(uint32_t timestamp){
	char title[32];
	struct tm* tmp;

	time_t t = (time_t) timestamp;
	t *= 60;
	tmp = localtime(&t);

	strftime(title, 32, "conv_ip_%d%m%y", tmp);
	char query[512];
	MYSQL_RES* res;
	res = mysql_list_tables(&db, title);
	if ( mysql_num_rows(res) > 0 ){
		logmsg(LOGLEVEL_DEBUG, "table %s exists, doing nothing", title);
	} else {
		logmsg(LOGLEVEL_DEBUG, "table %s does not exist, creating table", title);
		sprintf(query,"CREATE TABLE %s (timestamp INTEGER UNSIGNED, agent VARCHAR(16), input_if INTEGER UNSIGNED, output_if INTEGER UNSIGNED, src VARCHAR(16), dst VARCHAR(16), bytes INTEGER UNSIGNED, frames INTEGER UNSIGNED, CONSTRAINT %s_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,dst) ) ENGINE=myisam", title, title);
		logmsg(LOGLEVEL_DEBUG, "query: %s", query);
		mysql_query(&db, query);
	}
	table_conv_ip = timestamp/TABLE_INTERVAL;
	strncpy(table_conv_ip_name, title, 32);
}

void storage_mysql_create_conv_tcp(uint32_t timestamp){
	char title[32];
	struct tm* tmp;

	time_t t = (time_t) timestamp;
	t *= 60;
	tmp = localtime(&t);

	strftime(title, 32, "conv_tcp_%d%m%y", tmp);
	char query[512];
	MYSQL_RES* res;
	res = mysql_list_tables(&db, title);
	if ( mysql_num_rows(res) > 0 ){
		logmsg(LOGLEVEL_DEBUG, "table %s exists, doing nothing", title);
	} else {
		logmsg(LOGLEVEL_DEBUG, "table %s does not exist, creating table", title);
		sprintf(query, "CREATE TABLE %s ( timestamp INTEGER UNSIGNED, agent VARCHAR(16), input_if INTEGER UNSIGNED, output_if INTEGER UNSIGNED, src VARCHAR(16), sport INTEGER UNSIGNED, dst VARCHAR(16), dport INTEGER UNSIGNED, bytes INTEGER UNSIGNED, frames INTEGER UNSIGNED, CONSTRAINT %s_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,sport,dst,dport) ) ENGINE=myisam", title, title);
		logmsg(LOGLEVEL_DEBUG, "query: %s", query);
		mysql_query(&db, query);
	}
	table_conv_tcp = timestamp/TABLE_INTERVAL;
	strncpy(table_conv_tcp_name, title, 32);
}

void storage_mysql_create_conv_udp(uint32_t timestamp){
	char title[32];
	struct tm* tmp;

	time_t t = (time_t) timestamp;
	t *= 60;
	tmp = localtime(&t);

	strftime(title, 32, "conv_udp_%d%m%y", tmp);
	char query[512];
	MYSQL_RES* res;
	res = mysql_list_tables(&db, title);
	if ( mysql_num_rows(res) > 0 ){
		logmsg(LOGLEVEL_DEBUG, "table %s exists, doing nothing", title);
	} else {
		logmsg(LOGLEVEL_DEBUG, "table %s does not exist, creating table", title);
		sprintf(query, "CREATE TABLE %s ( timestamp INTEGER UNSIGNED, agent VARCHAR(16), input_if INTEGER UNSIGNED, output_if INTEGER UNSIGNED, src VARCHAR(16), sport INTEGER UNSIGNED, dst VARCHAR(16), dport INTEGER UNSIGNED, bytes INTEGER UNSIGNED, frames INTEGER UNSIGNED, CONSTRAINT %s_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,sport,dst,dport) ) ENGINE=myisam", title, title);
		logmsg(LOGLEVEL_DEBUG, "query: %s", query);
		mysql_query(&db, query);
	}
	table_conv_udp = timestamp/TABLE_INTERVAL;
	strncpy(table_conv_udp_name, title, 32);
}

void storage_mysql_create_counters(uint32_t timestamp){
	char title[32];
	struct tm* tmp;

	time_t t = (time_t) timestamp;
	t *= 60;
	tmp = localtime(&t);

	strftime(title, 32, "counters_%d%m%y", tmp);
	char* query = (char*) malloc(sizeof(char)*1024);
	MYSQL_RES* res;
	res = mysql_list_tables(&db, title);
	if ( mysql_num_rows(res) > 0 ){
		logmsg(LOGLEVEL_DEBUG, "table %s exists, doing nothing", title);
	} else {
		logmsg(LOGLEVEL_DEBUG, "table %s does not exist, creating table", title);
		sprintf(query, "CREATE TABLE %s (timestamp INTEGER UNSIGNED,agent VARCHAR(16),if_index INTEGER UNSIGNED,if_type	INTEGER UNSIGNED,if_speed BIGINT UNSIGNED,if_direction INTEGER UNSIGNED,if_if_status INTEGER UNSIGNED,if_in_octets BIGINT UNSIGNED,if_in_ucast_pkts INTEGER UNSIGNED,if_in_mcast_pkts INTEGER UNSIGNED,if_in_bcast_pkts INTEGER UNSIGNED,if_in_discards INTEGER UNSIGNED,if_in_errors INTEGER UNSIGNED,if_in_unknown_proto INTEGER UNSIGNED,if_out_octets BIGINT UNSIGNED,if_out_ucast_pkts INTEGER UNSIGNED,if_out_mcast_pkts INTEGER UNSIGNED,if_out_bcast_pkts INTEGER UNSIGNED,if_out_discards INTEGER UNSIGNED,if_out_errors INTEGER UNSIGNED,if_promisc INTEGER UNSIGNED,CONSTRAINT %s_pk PRIMARY KEY (timestamp,agent,if_index)) ENGINE=myisam", title, title);
		logmsg(LOGLEVEL_DEBUG, "query: %s", query);
		mysql_query(&db, query);
	}
	table_counters = timestamp/TABLE_INTERVAL;
	strncpy(table_counters_name, title, 32);
	free(query);
}

void storage_mysql_store_conv_ethernet(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){
	if(table_conv_ethernet < timestamp/TABLE_INTERVAL)
		storage_mysql_create_conv_ethernet(timestamp);

	uint32_t ethertype_ip = 0, ethertype_arp = 0, ethertype_rarp = 0, ethertype_802_1q = 0, ethertype_ipv6 = 0;
	uint32_t cnt= 0;
	uint32_t i;
	
	int fd = shm_open("mysql_tmp_ethernet", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
	
	char* buf;
	buf = (char*) malloc(sizeof(char)*1024);

	for(i=0; i<num;i++) {
		conv_list_t* l = list[i];
		if(l == NULL)
			continue;
		conv_list_node_t* n = l->data;
		while(n){
			
			conv_key_ethernet_t* k = (conv_key_ethernet_t*) n->key;
			conv_ethernet_t* c = (conv_ethernet_t*) n->conv;

			ethertype_ip 		+= c->protocols.ethertype_ip;
			ethertype_arp 		+= c->protocols.ethertype_arp;
			ethertype_rarp 		+= c->protocols.ethertype_rarp;
			ethertype_802_1q 	+= c->protocols.ethertype_802_1q;
			ethertype_ipv6 		+= c->protocols.ethertype_ipv6;

			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;

			char a[16];
			char src[18];
			char dst[18];
			strncpy(src, ether_ntoa((const struct ether_addr *)k->src), 18);
			strncpy(dst, ether_ntoa((const struct ether_addr *)k->dst), 18);
			num_to_ip(agent, a);
			sprintf(buf, "%u|%s|%u|%u|%s|%s|%u|%u\n",
				timestamp,
				a,
				k->sflow_input_if,
				k->sflow_output_if,
				src,
				dst,
				c->bytes, 
				c->frames
		   	);
			write(fd, buf, strlen(buf));
			cnt++;
		}
	}

	char stmt[256];
	sprintf(stmt, "LOAD DATA INFILE '%s/mysql_tmp_ethernet' INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n'", PATH_SHM, table_conv_ethernet_name);

	char stmt_alter_enable[32];
	sprintf(stmt_alter_enable, "ALTER TABLE %s ENABLE KEYS", table_conv_ethernet_name);
	char stmt_alter_disable[32];
	sprintf(stmt_alter_enable, "ALTER TABLE %s DISABLE KEYS", table_conv_ethernet_name);

	mysql_query(&db, "FLUSH TABLES");	
	mysql_query(&db, stmt_alter_disable);
	if(mysql_query(&db, stmt) != 0)
		logmsg(LOGLEVEL_DEBUG, "ERROR LOADING INFILE: %s", mysql_error(&db));
	mysql_query(&db, stmt_alter_enable);
	mysql_query(&db, "FLUSH TABLES");	

	free(buf);
	close(fd);
	shm_unlink("mysql_tmp_ethernet");
	logmsg(LOGLEVEL_DEBUG, "Stored %u ethernet conversations (ip:%u, arp:%u, rarp:%u, 802_1q:%u, ipv6:%u)", cnt, ethertype_ip, ethertype_arp, ethertype_rarp, ethertype_802_1q, ethertype_ipv6 );
}

void storage_mysql_store_conv_ip(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){

	if(table_conv_ip < timestamp/TABLE_INTERVAL)
		 storage_mysql_create_conv_ip(timestamp);

	uint32_t cnt = 0;
	uint32_t i;

	int fd = shm_open("mysql_tmp_ip", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
	
	char* buf;
	buf = (char*) malloc(sizeof(char)*1024);

	uint32_t ip_icmp = 0,
			 ip_tcp = 0,
			 ip_udp = 0;

	for(i=0; i<num; i++){
		conv_list_t* l = list[i];

		if(l == NULL)
			continue;

		conv_list_node_t* n = l->data;
		while(n){
			conv_key_ip_t* k = (conv_key_ip_t*) n->key;
			conv_ip_t* c = (conv_ip_t*) n->conv;

			ip_icmp += c->protocol[1];
			ip_tcp  += c->protocol[6];
			ip_udp  += c->protocol[17];

			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;

			char src[16];
			char dst[16];
			char a[16];
			num_to_ip(agent, a);
			num_to_ip(k->src, src);
			num_to_ip(k->dst, dst);

			sprintf(buf, "%u|%s|%u|%u|%s|%s|%u|%u\n", 
				timestamp,
				a,
				k->sflow_input_if,
				k->sflow_output_if,
				src,
				dst,
				c->bytes,
				c->frames
			);
			write(fd, buf, strlen(buf));
			cnt++;
		}
	}

	char stmt[256];
	sprintf(stmt, "LOAD DATA INFILE '%s/mysql_tmp_ip' INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n'", PATH_SHM, table_conv_ip_name);
	       
	if(mysql_query(&db, stmt) != 0)
		logmsg(LOGLEVEL_DEBUG, "ERROR LOADING INFILE: %s", mysql_error(&db));

	free(buf);
	close(fd);
	shm_unlink("mysql_tmp_ip");

	logmsg(LOGLEVEL_DEBUG, "Stored %u ip conversations (icmp:%u, tcp:%u, udp:%u)", cnt, ip_icmp, ip_tcp, ip_udp);
}

void storage_mysql_store_conv_tcp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){

	if(table_conv_tcp < timestamp/TABLE_INTERVAL)
		 storage_mysql_create_conv_tcp(timestamp);

	uint32_t cnt= 0;
	uint32_t i;

	int fd = shm_open("mysql_tmp_tcp", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);

	char* buf;
	buf = (char*) malloc(sizeof(char)*1024);

	uint32_t tcp_urg = 0,
			 tcp_ack = 0,
			 tcp_psh = 0,
			 tcp_rst = 0,
			 tcp_syn = 0,
			 tcp_fin = 0;

	for(i=0; i<num; i++){
		conv_list_t* l = list[i];

		if(l == NULL)
			continue;

		conv_list_node_t* n = l->data;
		while(n){
			conv_key_tcp_t* k = (conv_key_tcp_t*) n->key;
			conv_tcp_t* c = (conv_tcp_t*) n->conv;

			tcp_urg += c->flags[TCP_URG];
			tcp_ack += c->flags[TCP_ACK];
			tcp_psh += c->flags[TCP_PSH];
			tcp_rst += c->flags[TCP_RST];
			tcp_syn += c->flags[TCP_SYN];
			tcp_fin += c->flags[TCP_FIN];

			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;

			char src[16];
			char dst[16];
			char a[16];
			num_to_ip(agent, a);
			num_to_ip(k->src, src);
			num_to_ip(k->dst, dst);

			sprintf(buf, "%u|%s|%u|%u|%s|%u|%s|%u|%u|%u\n",
				timestamp,
				a,
				k->sflow_input_if,
				k->sflow_output_if,
				src,
				k->src_port,
				dst,
				k->dst_port,
				c->bytes,
				c->frames
			);
			write(fd, buf, strlen(buf));
			cnt++;
		}
	}

	char stmt[256];
	sprintf(stmt, "LOAD DATA INFILE '%s/mysql_tmp_tcp' INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n'", PATH_SHM, table_conv_tcp_name);
	       
	if(mysql_query(&db, stmt) != 0)
		logmsg(LOGLEVEL_DEBUG, "ERROR LOADING INFILE: %s", mysql_error(&db));

	free(buf);
	close(fd);
	shm_unlink("mysql_tmp_tcp");

	logmsg(LOGLEVEL_DEBUG, "Stored %u tcp conversations (urg:%u, ack:%u, psh:%u, rst:%u, syn:%u, fin:%u)", cnt, tcp_urg, tcp_ack, tcp_psh, tcp_rst, tcp_syn, tcp_fin);
}

void storage_mysql_store_conv_udp(conv_list_t** list, uint32_t num, uint32_t agent, uint32_t timestamp){

	if(table_conv_udp < timestamp/TABLE_INTERVAL)
		storage_mysql_create_conv_udp(timestamp);

	uint32_t cnt= 0;
	uint32_t i;
	
	int fd = shm_open("mysql_tmp_udp", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);
	
	char* buf;
	buf = (char*) malloc(sizeof(char)*1024);

	for(i=0; i<num;i++) {
		conv_list_t* l = list[i];

		if(l == NULL)
			continue;

		conv_list_node_t* n = l->data;
		while(n){
			conv_key_udp_t* k = (conv_key_udp_t*) n->key;
			conv_udp_t* c = (conv_udp_t*) n->conv;

			conv_list_node_t* tmp;
			tmp = n;
			n = n->next;

			char src[16];
			char dst[16];
			char a[16];
			num_to_ip(agent, a);
			num_to_ip(k->src, src);
			num_to_ip(k->dst, dst);

			sprintf(buf, "%u|%s|%u|%u|%s|%u|%s|%u|%u|%u\n",
				timestamp,
				a,
				k->sflow_input_if,
				k->sflow_output_if,
				src,
				k->src_port,
				dst,
				k->dst_port,
				c->bytes,
				c->frames
			);
			write(fd, buf, strlen(buf));
			cnt++;
		}
	}

	char stmt[256];
	sprintf(stmt, "LOAD DATA INFILE '%s/mysql_tmp_udp' INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n'", PATH_SHM, table_conv_udp_name);
	       
	if(mysql_query(&db, stmt) != 0)
		logmsg(LOGLEVEL_DEBUG, "ERROR LOADING INFILE: %s", mysql_error(&db));

	free(buf);
	close(fd);
	shm_unlink("mysql_tmp_udp");

	logmsg(LOGLEVEL_DEBUG, "Stored %u udp conversations", cnt);
}

void storage_mysql_store_cntr(counter_list_t* list, uint32_t timestamp){

	if(table_counters < timestamp/TABLE_INTERVAL)
		 storage_mysql_create_counters(timestamp);

	char* query;
	char* ptr;
	query = (char*) malloc(sizeof(char)*BULK_INSERT_NUM*BULK_INSERT_SIZE_CNTR);

	char stmt[] = "INSERT INTO %s (\
				timestamp,agent,if_index,if_type,if_speed,if_direction,if_if_status,\
				if_in_octets,if_in_ucast_pkts,if_in_mcast_pkts,if_in_bcast_pkts,\
				if_in_discards,if_in_errors,if_in_unknown_proto,if_out_octets,\
				if_out_ucast_pkts,if_out_mcast_pkts,if_out_bcast_pkts,if_out_discards,\
				if_out_errors,if_promisc) VALUES ";

	ptr = query;
	memset(query, 0, sizeof(char)*BULK_INSERT_NUM*BULK_INSERT_SIZE_CNTR);
	ptr += sizeof(char) * sprintf(ptr, stmt, table_counters_name);

	counter_list_node_t* node = list->data;

	uint32_t cnt = 0;

	while(node != NULL){
		SFCntrSample* s = &node->sample;

		char a[16];
		num_to_ip(s->agent_address, a);

		ptr += sizeof(char) * sprintf(ptr,	"(%u, '%s', %u, %u, %llu, %u, %u, %llu, %u, %u, %u, %u, %u, %u, %llu, %u, %u, %u, %u, %u, %u),",
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

		cnt++;

		if(cnt%BULK_INSERT_NUM == 0){
			*(--ptr) = ' ';
			mysql_query(&db, query);
			ptr = query;
			memset(query, 0, sizeof(char) * BULK_INSERT_NUM * BULK_INSERT_SIZE_CNTR); 
			ptr += sizeof(char) * sprintf(ptr, stmt, table_counters_name);
		}

		node = node->next;
	}
	*(--ptr) = ' ';
	mysql_query(&db, query);
	free(query);
	logmsg(LOGLEVEL_DEBUG, "Stored %u counter samples", cnt);
}

storage_module_t storage_mod_mysql = {
	.name 					= "mysql",
	.init 					= storage_mysql_init,
	.destroy 				= storage_mysql_destroy,
	.store_cntr 			= storage_mysql_store_cntr,
	.store_conv_ethernet 	= storage_mysql_store_conv_ethernet,
	.store_conv_ip 			= storage_mysql_store_conv_ip,
	.store_conv_tcp 		= storage_mysql_store_conv_tcp,
	.store_conv_udp 		= storage_mysql_store_conv_udp
};

void storage_mysql_load(){
	logmsg(LOGLEVEL_DEBUG, "Registering mysql module");
	storage_modules_register(&storage_mod_mysql);
}
