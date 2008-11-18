DROP TABLE IF EXISTS conv_ethernet;
DROP TABLE IF EXISTS conv_ip;
DROP TABLE IF EXISTS conv_tcp;
DROP TABLE IF EXISTS conv_udp;

CREATE TABLE conv_ethernet(
	id 			INTEGER NOT NULL AUTO_INCREMENT,
	timestamp 	INTEGER,
	agent 		VARCHAR(16),
	input_if 	INTEGER,
	output_if 	INTEGER,
	src 		VARCHAR(18),
	dst 		VARCHAR(18),
	bytes 		INTEGER,
	frames 		INTEGER,
	PRIMARY KEY(id)
);

CREATE TABLE conv_ip(
	id 			INTEGER NOT NULL AUTO_INCREMENT,
	timestamp 	INTEGER,
	agent 		VARCHAR(16),
	input_if 	INTEGER,
	output_if 	INTEGER,
	src 		VARCHAR(16),
	dst 		VARCHAR(16),
	bytes 		INTEGER,
	packets		INTEGER,
	PRIMARY KEY(id)
);

CREATE TABLE conv_tcp(
	id 			INTEGER NOT NULL AUTO_INCREMENT,
	timestamp 	INTEGER,
	agent 		VARCHAR(16),
	input_if 	INTEGER,
	output_if 	INTEGER,
	src 		VARCHAR(16),
	sport		INTEGER,
	dst 		VARCHAR(16),
	dport		INTEGER,
	bytes 		INTEGER,
	segments	INTEGER,
	PRIMARY KEY(id)
);

CREATE TABLE conv_udp(
	id 			INTEGER NOT NULL AUTO_INCREMENT,
	timestamp 	INTEGER,
	agent 		VARCHAR(16),
	input_if 	INTEGER,
	output_if 	INTEGER,
	src 		VARCHAR(16),
	sport		INTEGER,
	dst 		VARCHAR(16),
	dport		INTEGER,
	bytes 		INTEGER,
	segments	INTEGER,
	PRIMARY KEY(id)
);
