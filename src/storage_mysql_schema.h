#define CREATE_CONV_ETHERNET_SCHEMA "                  								\
    CREATE TABLE %s (                                                            	\
        id          INTEGER NOT NULL AUTO_INCREMENT,                                \
        timestamp   INTEGER UNSIGNED,                                               \
        agent       VARCHAR(16),                                                    \
        input_if    INTEGER UNSIGNED,                                               \
        output_if   INTEGER UNSIGNED,                                               \
        src         VARCHAR(18),                                                    \
        dst         VARCHAR(18),                                                    \
        bytes       INTEGER UNSIGNED,                                               \
        frames      INTEGER UNSIGNED,                                               \
        srate       INTEGER UNSIGNED,                                               \
        CONSTRAINT %s_pk PRIMARY KEY (id),                                     		\
        INDEX idx_%s (timestamp, agent, input_if, output_if, src, dst, bytes)  		\
    ) ENGINE=myisam"

#define CREATE_CONV_IP_SCHEMA "			                                            \
    CREATE TABLE %s ( 	                                                            \
        id          INTEGER NOT NULL AUTO_INCREMENT,                                \
        timestamp   INTEGER UNSIGNED,                                               \
        agent       INTEGER UNSIGNED,                                               \
        input_if    INTEGER UNSIGNED,                                               \
        output_if   INTEGER UNSIGNED,                                               \
        src         INTEGER UNSIGNED,                                               \
        dst         INTEGER UNSIGNED,                                               \
        bytes       INTEGER UNSIGNED,                                               \
        frames      INTEGER UNSIGNED,                                               \
        srate       INTEGER UNSIGNED,                                               \
        CONSTRAINT %s_pk PRIMARY KEY (id),                                     		\
        INDEX idx_%s (timestamp, agent, input_if, output_if, src, dst, bytes)  		\
    ) ENGINE=myisam"

#define CREATE_CONV_TCP_SCHEMA "	                                                \
    CREATE TABLE %s (                                                               \
        id          INTEGER NOT NULL AUTO_INCREMENT,                                \
        timestamp   INTEGER UNSIGNED,                                               \
        agent       INTEGER UNSIGNED,                                               \
        input_if    INTEGER UNSIGNED,                                               \
        output_if   INTEGER UNSIGNED,                                               \
        src         INTEGER UNSIGNED,                                               \
        sport       INTEGER UNSIGNED,                                               \
        dst         INTEGER UNSIGNED,                                               \
        dport       INTEGER UNSIGNED,                                               \
        bytes       INTEGER UNSIGNED,                                               \
        frames      INTEGER UNSIGNED,                                               \
        srate       INTEGER UNSIGNED,                                               \
        CONSTRAINT %s_pk PRIMARY KEY (id),                                          \
        INDEX idx_%s (timestamp, agent, input_if, output_if, src, dst, bytes)       \
    ) ENGINE=myisam"

#define CREATE_CONV_UDP_SCHEMA "                                                    \
    CREATE TABLE %s (                                                               \
        id          INTEGER NOT NULL AUTO_INCREMENT,                                \
        timestamp   INTEGER UNSIGNED,                                               \
        agent       INTEGER UNSIGNED,                                               \
        input_if    INTEGER UNSIGNED,                                               \
        output_if   INTEGER UNSIGNED,                                               \
        src         INTEGER UNSIGNED,                                               \
        sport       INTEGER UNSIGNED,                                               \
        dst         INTEGER UNSIGNED,                                               \
        dport       INTEGER UNSIGNED,                                               \
        bytes       INTEGER UNSIGNED,                                               \
        frames      INTEGER UNSIGNED,                                               \
        srate       INTEGER UNSIGNED,                                               \
        CONSTRAINT %s_pk PRIMARY KEY (id),                                     		\
        INDEX idx_%s (timestamp, agent, input_if, output_if, src, dst, bytes)  		\
    ) ENGINE=myisam"

#define CREATE_COUNTERS_SCHEMA "  		                            \
    CREATE TABLE %s (	                                            \
        id                      INTEGER NOT NULL AUTO_INCREMENT,    \
        timestamp               INTEGER UNSIGNED,                   \
        agent                   INTEGER UNSIGNED,                   \
        if_index                INTEGER UNSIGNED,                   \
        if_type                 INTEGER UNSIGNED,                   \
        if_speed                BIGINT  UNSIGNED,                   \
        if_direction            INTEGER UNSIGNED,                   \
        if_if_status            INTEGER UNSIGNED,                   \
        if_in_octets            BIGINT  UNSIGNED,                   \
        if_in_ucast_pkts        INTEGER UNSIGNED,                   \
        if_in_mcast_pkts        INTEGER UNSIGNED,                   \
        if_in_bcast_pkts        INTEGER UNSIGNED,                   \
        if_in_discards          INTEGER UNSIGNED,                   \
        if_in_errors            INTEGER UNSIGNED,                   \
        if_in_unknown_proto     INTEGER UNSIGNED,                   \
        if_out_octets           BIGINT  UNSIGNED,                   \
        if_out_ucast_pkts       INTEGER UNSIGNED,                   \
        if_out_mcast_pkts       INTEGER UNSIGNED,                   \
        if_out_bcast_pkts       INTEGER UNSIGNED,                   \
        if_out_discards         INTEGER UNSIGNED,                   \
        if_out_errors           INTEGER UNSIGNED,                   \
        if_promisc              INTEGER UNSIGNED,                   \
        CONSTRAINT %s_pk PRIMARY KEY (id),                     		\
        INDEX idx_%s (timestamp, agent, if_index)              		\
    ) ENGINE=myisam"

#define LOAD_INFILE_ETHERNET_SCHEMA "										\
	LOAD DATA INFILE '%s/mysql_tmp_ethernet' 								\
	INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n' 		\
	(timestamp,agent,input_if,output_if,src,dst,bytes,frames,srate)"

#define LOAD_INFILE_IP_SCHEMA "												\
	LOAD DATA INFILE '%s/mysql_tmp_ip' 										\
	INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n' 		\
	(timestamp,agent,input_if,output_if,src,dst,bytes,frames,srate)"

#define LOAD_INFILE_TCP_SCHEMA "											\
	LOAD DATA INFILE '%s/mysql_tmp_tcp' 									\
	INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n' 		\
	(timestamp,agent,input_if,output_if,src,sport,dst,dport,bytes,frames,srate)"

#define LOAD_INFILE_UDP_SCHEMA "											\
	LOAD DATA INFILE '%s/mysql_tmp_udp' 									\
	INTO TABLE %s FIELDS TERMINATED BY '|' LINES TERMINATED BY '\\n' 		\
	(timestamp,agent,input_if,output_if,src,sport,dst,dport,bytes,frames,srate)"
