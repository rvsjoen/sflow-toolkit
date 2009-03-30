DROP TABLE IF EXISTS conv_ethernet;
DROP TABLE IF EXISTS conv_ip;
DROP TABLE IF EXISTS conv_tcp;
DROP TABLE IF EXISTS conv_udp;
DROP TABLE IF EXISTS counters;

CREATE TABLE conv_ethernet(
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(18),
	dst 		VARCHAR(18),
	bytes 		INTEGER UNSIGNED,
	frames 		INTEGER UNSIGNED,
	CONSTRAINT conv_ethernet_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,dst)
) ENGINE=innodb;

CREATE TABLE conv_ip(
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	dst 		VARCHAR(16),
	bytes 		INTEGER UNSIGNED,
	frames		INTEGER UNSIGNED,
	CONSTRAINT conv_ip_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,dst)
) ENGINE=innodb;

CREATE TABLE conv_tcp(
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	sport		INTEGER UNSIGNED,
	dst 		VARCHAR(16),
	dport		INTEGER UNSIGNED,
	bytes 		INTEGER UNSIGNED,
	frames		INTEGER UNSIGNED,
	CONSTRAINT conv_tcp_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,sport,dst,dport)
) ENGINE=innodb;

CREATE TABLE conv_udp(
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	sport		INTEGER UNSIGNED,
	dst 		VARCHAR(16),
	dport		INTEGER UNSIGNED,
	bytes 		INTEGER UNSIGNED,
	frames		INTEGER UNSIGNED,
	CONSTRAINT conv_udp_pk PRIMARY KEY (timestamp,agent,input_if,output_if,src,sport,dst,dport)
) ENGINE=innodb;

CREATE TABLE counters (
	timestamp				INTEGER UNSIGNED,
	agent					VARCHAR(16),
	if_index				INTEGER UNSIGNED,
	if_type					INTEGER UNSIGNED,
	if_speed				BIGINT UNSIGNED,
	if_direction			INTEGER UNSIGNED,
	if_if_status			INTEGER UNSIGNED,
	if_in_octets			BIGINT UNSIGNED,
	if_in_ucast_pkts 		INTEGER UNSIGNED,
	if_in_mcast_pkts 		INTEGER UNSIGNED,
	if_in_bcast_pkts 		INTEGER UNSIGNED,
	if_in_discards 			INTEGER UNSIGNED,
	if_in_errors 			INTEGER UNSIGNED,
	if_in_unknown_proto 	INTEGER UNSIGNED,
	if_out_octets 			BIGINT UNSIGNED,
	if_out_ucast_pkts 		INTEGER UNSIGNED,
	if_out_mcast_pkts 		INTEGER UNSIGNED,
	if_out_bcast_pkts 		INTEGER UNSIGNED,
	if_out_discards 		INTEGER UNSIGNED,
	if_out_errors 			INTEGER UNSIGNED,
	if_promisc				INTEGER UNSIGNED,
	CONSTRAINT counters_pk PRIMARY KEY (timestamp,agent,if_index)
) ENGINE=innodb;
