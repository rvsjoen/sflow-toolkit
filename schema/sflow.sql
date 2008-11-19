DROP TABLE IF EXISTS conv_ethernet;
DROP TABLE IF EXISTS conv_ip;
DROP TABLE IF EXISTS conv_tcp;
DROP TABLE IF EXISTS conv_udp;
DROP TABLE IF EXISTS counters;

CREATE TABLE conv_ethernet(
	id 			SERIAL,
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(18),
	dst 		VARCHAR(18),
	bytes 		INTEGER UNSIGNED,
	frames 		INTEGER UNSIGNED,
	PRIMARY KEY(id)
);

CREATE TABLE conv_ip(
	id 			SERIAL,
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	dst 		VARCHAR(16),
	bytes 		INTEGER UNSIGNED,
	packets		INTEGER UNSIGNED,
	PRIMARY KEY(id)
);

CREATE TABLE conv_tcp(
	id 			SERIAL,
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	sport		INTEGER UNSIGNED,
	dst 		VARCHAR(16),
	dport		INTEGER UNSIGNED,
	bytes 		INTEGER UNSIGNED,
	segments	INTEGER UNSIGNED,
	PRIMARY KEY(id)
);

CREATE TABLE conv_udp(
	id 			SERIAL,
	timestamp 	INTEGER UNSIGNED,
	agent 		VARCHAR(16),
	input_if 	INTEGER UNSIGNED,
	output_if 	INTEGER UNSIGNED,
	src 		VARCHAR(16),
	sport		INTEGER UNSIGNED,
	dst 		VARCHAR(16),
	dport		INTEGER UNSIGNED,
	bytes 		INTEGER UNSIGNED,
	segments	INTEGER UNSIGNED,
	PRIMARY KEY(id)
);

CREATE TABLE counters (
	id						SERIAL,
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
	PRIMARY KEY(id)
);
