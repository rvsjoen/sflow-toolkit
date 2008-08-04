#ifndef __sflow_parser_h__
#define __sflow_parser_h__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "sflow.h"

#define RAW_HEADER_SIZE 128

typedef enum _SFSample_t {
	SFTYPE_FLOW = 0,
	SFTYPE_CNTR = 1
} SFSample_t;

typedef struct _SFDatagram {
        time_t timestamp;
        /* the raw pdu and some pointers to ease navigation*/
        uint8_t* raw_sample;
        uint8_t* raw_start;
        uint8_t* raw_end;
        uint32_t raw_length;
        /* decode cursor */
        uint32_t * data;
} SFDatagram;

typedef struct _SFLSample_hdr {
        uint32_t tag;
        uint32_t length;
} SFLSample_hdr;

typedef struct _SFLFlowRecord_hdr {
        uint32_t tag;
        uint32_t length;
} SFLFlowRecord_hdr;

typedef struct _SFLCounterRecord_hdr {
        uint32_t tag;
        uint32_t length;
} SFLCounterRecord_hdr;

typedef struct _SFSample {
        // Datagram
        time_t timestamp;
        uint32_t agent_address;
        uint32_t sub_agent_id;

        // Sample header
        uint32_t sample_tag_enterprise;
        uint32_t sample_tag_format;
        uint32_t sample_length;
} SFSample;

typedef struct _SFFlowSample {
        // Datagram
        time_t timestamp;
        uint32_t agent_address;
        uint32_t sub_agent_id;

        // Sample header
        // uint32_t sample_tag_enterprise;
        // uint32_t sample_tag_format;
        // uint32_t sample_length;

        // Flow sample
        uint32_t sample_sequence_number;
        uint32_t sample_source_id_type;
        uint32_t sample_source_id_index;
        uint32_t sample_sampling_rate;
        uint32_t sample_sample_pool;
        uint32_t sample_drops;
        uint32_t sample_input_if_format;
        uint32_t sample_input_if_value;
        uint32_t sample_output_if_format;
        uint32_t sample_output_if_value;

        // Record Header
        // uint32_t record_tag_enterprise;
        // uint32_t record_tag_format;
        // uint32_t record_length;

        // Sampled raw packetheader
        uint32_t raw_header_protocol;
        uint32_t raw_header_frame_length;
        uint32_t raw_header_stripped;
        uint32_t raw_header_length;
        uint8_t raw_header[RAW_HEADER_SIZE];
} SFFlowSample;


typedef struct _SFCntrSample {
        // Datagram
        time_t timestamp;
        uint32_t agent_address;
        uint32_t sub_agent_id;

        // Sample header
        // uint32_t sample_tag_enterprise;
        // uint32_t sample_tag_format;
        // uint32_t sample_length;
		
		// Counter sample
        uint32_t sample_sequence_number;
        uint32_t sample_source_id_type;
        uint32_t sample_source_id_index;

        // Record Header
        // uint32_t record_tag_enterprise;
        // uint32_t record_tag_format;
        // uint32_t record_length;

		// Generic counters
		uint32_t counter_generic_if_index;
		uint32_t counter_generic_if_type;
		uint64_t counter_generic_if_speed;
		uint32_t counter_generic_if_direction;
		uint32_t counter_generic_if_if_status;

		uint64_t counter_generic_if_in_octets;
		uint32_t counter_generic_if_in_ucast_pkts;
		uint32_t counter_generic_if_in_mcast_pkts;
		uint32_t counter_generic_if_in_bcast_pkts;
		uint32_t counter_generic_if_in_discards;
		uint32_t counter_generic_if_in_errors;
		uint32_t counter_generic_if_in_unknown_proto;

		uint64_t counter_generic_if_out_octets;
		uint32_t counter_generic_if_out_ucast_pkts;
		uint32_t counter_generic_if_out_mcast_pkts;
		uint32_t counter_generic_if_out_bcast_pkts;
		uint32_t counter_generic_if_out_discards;
		uint32_t counter_generic_if_out_errors;
		uint32_t counter_generic_if_promisc;

		// Ethernet counters
		uint32_t counter_ethernet_dot3_stats_AlignmentErrors;
		uint32_t counter_ethernet_dot3_stats_FCSErrors;
		uint32_t counter_ethernet_dot3_stats_SingleCollisionFrames;
		uint32_t counter_ethernet_dot3_stats_MultipleCollisionFrames;
		uint32_t counter_ethernet_dot3_stats_SQETestErrors;
		uint32_t counter_ethernet_dot3_stats_DeferredTransmissions;
		uint32_t counter_ethernet_dot3_stats_LateCollisions;
		uint32_t counter_ethernet_dot3_stats_ExcessiveCollisions;
		uint32_t counter_ethernet_dot3_stats_InternalMacTransmitErrors;
		uint32_t counter_ethernet_dot3_stats_CarrierSenseErrors;
		uint32_t counter_ethernet_dot3_stats_FrameTooLongs;
		uint32_t counter_ethernet_dot3_stats_InternalMacReceiveErrors;
		uint32_t counter_ethernet_dot3_stats_SymbolErrors;

} SFCntrSample;

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  peekData32
 *  Description:  Same as getData32 except we do not move the data pointer
 * =====================================================================================
 */
u_int32_t peekData32(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  peekData32_nobswap
 *  Description:  Same as getData32_nobswap except we do not move the data pointer
 * =====================================================================================
 */
u_int32_t peekData32_nobswap(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getData32
 *  Description:  Return the next 32-bit integer from the SFDatagram converting it from
 *  network byter order to host byte order and move the data pointer in the datagram
 * =====================================================================================
 */
u_int32_t getData32(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getData32_nobswap
 *  Description:  Return the next 32-bit integer from the SFDatagram and move the
 *  data pointer in the datagram without swapping endian-ness
 * =====================================================================================
 */
u_int32_t getData32_nobswap(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  skipBytes
 *  Description:  Move the data pointer in the SFDatagram 'skip' bytes forward
 * =====================================================================================
 */
void skipBytes(SFDatagram *sample, uint32_t skip);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getAddress
 *  Description:  Return the agent address as a 32-bit uint32_teger and move the
 *  data pointer in the datagram relative to the address type
 * =====================================================================================
 */
u_int32_t getAddress(SFDatagram *sample, SFLAddress *address);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  parseCountersGeneric
 *  Description:  Parse generic counter record
 * =====================================================================================
 */
void parseCountersGeneric(SFDatagram* datagram, SFCntrSample* sample);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  parseCountersEthernet
 *  Description:  Parse ethernet counter record
 * =====================================================================================
 */
void parseCountersEthernet(SFDatagram* datagram, SFCntrSample* sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printSampledHeader
 *  Description:  Print the information contained in a SFLSampled_header structure
 * =====================================================================================
 */
void printSampledHeader(SFLSampled_header* hdr);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  parseSampledHeader
 *  Description:  Parse the sampled raw header
 * =====================================================================================
 */
void parseSampledHeader(SFDatagram* datagram, SFFlowSample* sample);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printCounterRecordHeader
 *  Description:  Print the information in a coutner record
 * =====================================================================================
 */
void printCounterRecordHeader(SFLCounterRecord_hdr* hdr);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  parseCounterRecordHeader
 *  Description:  Parse the header of a counter sample record
 * =====================================================================================
 */
void parseCounterRecordHeader(SFDatagram* datagram, SFCntrSample* sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printFlowRecordHeader
 *  Description:  Print the information contained in a SFLFlowRecord_hdr structure
 * =====================================================================================
 */
void printFlowRecordHeader(SFLFlowRecord_hdr* hdr);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  parseFlowRecordHeader
 *  Description:  Parse the flow record header and extract the data
 * =====================================================================================
 */
void parseFlowRecordHeader(SFDatagram* datagram, SFFlowSample* sample);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printCounterSample
 *  Description:  Print the information in a SFLCounters_sample_expanded structure
 * =====================================================================================
 */
void printCounterSample(SFLCounters_sample_expanded* s);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  parseCounterSample
 *  Description:  Parse the counter sample
 * =====================================================================================
 */
void parseCounterSample(SFDatagram* datagram, SFCntrSample* sample, bool expanded);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printFlowSample
 *  Description:  Print the information in a SFLFlow_sample structure
 * =====================================================================================
 */
void printFlowSample(SFLFlow_sample_expanded* s);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  parseFlowSample
 *  Description:  Parse the sample, extract the information into a
 *  SFLFlow_sample_expanded structure and call the function to parse the record header
 * =====================================================================================
 */
void parseFlowSample(SFDatagram* datagram, SFFlowSample* sample, bool expanded);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printSampleHeader
 *  Description:  Print out the information in the SFLSample_hdr structure
 * =====================================================================================
 */
void printSampleHeader(SFLSample_hdr* hdr);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  parseSample
 *  Description:  Parse the sample header and pass control to the correct parser
 *  based on the sample tag
 * =====================================================================================
 */
void parseSample(SFDatagram* datagram, SFSample* sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printDatagramHeader
 *  Description:  Print out all the information in the datagram header structure
 * =====================================================================================
 */
void printDatagramHeader(const SFLSample_datagram_hdr* hdr);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  parseDatagram
 *  Description:  Populate the datagram structure with the pointers and a timestamp
 *  Then we populate the SFLSample_datagram_hdr structure and extract the fields we want
 *  from it to build a template for the samples we intend to store
 *  Then we parse each sample and store it
 * =====================================================================================
 */
void parseDatagram(uint8_t* data, uint32_t n);

#endif
