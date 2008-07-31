#ifndef __sflow_parser_h__
#define __sflow_parser_h__

#include <stdint.h>
#include "sflow.h"

// Ethernet headers (14 byte) + IP headers (20 byte) + TCP headers (20 byte)
const int RAW_HEADER_SIZE = 128;

enum SFSample_t {
	SFTYPE_FLOW = 0,
	SFTYPE_CNTR = 1
};

typedef struct _SFDatagram {
        time_t timestamp;
        /* the raw pdu and some pointers to ease navigation*/
        unsigned char* raw_sample;
        unsigned char* raw_start;
        unsigned char* raw_end;
        unsigned int raw_length;
        /* decode cursor */
        unsigned int * data;
} SFDatagram;

typedef struct _SFLSample_hdr {
        unsigned int tag;
        unsigned int length;
} SFLSample_hdr;

typedef struct _SFLFlowRecord_hdr {
        unsigned int tag;
        unsigned int length;
} SFLFlowRecord_hdr;

typedef struct _SFLCounterRecord_hdr {
        unsigned int tag;
        unsigned int length;
} SFLCounterRecord_hdr;

typedef struct _SFSample {
        // Datagram
        time_t timestamp;
        unsigned int agent_address;
        unsigned int sub_agent_id;

        // Sample header
        unsigned int sample_tag_enterprise;
        unsigned int sample_tag_format;
        unsigned int sample_length;
} SFSample;

typedef struct _SFFlowSample {
        // Datagram
        time_t timestamp;
        unsigned int agent_address;
        unsigned int sub_agent_id;

        // Sample header
        // unsigned int sample_tag_enterprise;
        // unsigned int sample_tag_format;
        // unsigned int sample_length;

        // Flow sample
        unsigned int sample_sequence_number;
        unsigned int sample_source_id_type;
        unsigned int sample_source_id_index;
        unsigned int sample_sampling_rate;
        unsigned int sample_sample_pool;
        unsigned int sample_drops;
        unsigned int sample_input_if_format;
        unsigned int sample_input_if_value;
        unsigned int sample_output_if_format;
        unsigned int sample_output_if_value;

        // Record Header
        // unsigned int record_tag_enterprise;
        // unsigned int record_tag_format;
        // unsigned int record_length;

        // Sampled raw packetheader
        unsigned int raw_header_protocol;
        unsigned int raw_header_frame_length;
        unsigned int raw_header_stripped;
        unsigned int raw_header_length;
        unsigned char raw_header[RAW_HEADER_SIZE];
} SFFlowSample;


typedef struct _SFCntrSample {
        // Datagram
        time_t timestamp;
        unsigned int agent_address;
        unsigned int sub_agent_id;

        // Sample header
        // unsigned int sample_tag_enterprise;
        // unsigned int sample_tag_format;
        // unsigned int sample_length;
		
		// Counter sample
        unsigned int sample_sequence_number;
        unsigned int sample_source_id_type;
        unsigned int sample_source_id_index;

        // Record Header
        // unsigned int record_tag_enterprise;
        // unsigned int record_tag_format;
        // unsigned int record_length;

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
static u_int32_t peekData32(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  peekData32_nobswap
 *  Description:  Same as getData32_nobswap except we do not move the data pointer
 * =====================================================================================
 */
static u_int32_t peekData32_nobswap(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getData32
 *  Description:  Return the next 32-bit integer from the SFDatagram converting it from
 *  network byter order to host byte order and move the data pointer in the datagram
 * =====================================================================================
 */
static u_int32_t getData32(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getData32_nobswap
 *  Description:  Return the next 32-bit integer from the SFDatagram and move the
 *  data pointer in the datagram without swapping endian-ness
 * =====================================================================================
 */
static u_int32_t getData32_nobswap(SFDatagram *sample);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  skipBytes
 *  Description:  Move the data pointer in the SFDatagram 'skip' bytes forward
 * =====================================================================================
 */
static void skipBytes(SFDatagram *sample, int skip);

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getAddress
 *  Description:  Return the agent address as a 32-bit unsigned integer and move the
 *  data pointer in the datagram relative to the address type
 * =====================================================================================
 */
static u_int32_t getAddress(SFDatagram *sample, SFLAddress *address);

void parseCountersGeneric(SFDatagram* datagram, SFCntrSample* sample);
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

void printCounterRecordHeader(SFLCounterRecord_hdr* hdr);
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

void printCounterSample(SFLCounters_sample_expanded* s);
void parseCounterSample(SFDatagram* datagram, SFCntrSample* sample, bool expanded=false);

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
void parseFlowSample(SFDatagram* datagram, SFFlowSample* sample, bool expanded=false);

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
void parseDatagram(unsigned char* data, int n);

#endif
