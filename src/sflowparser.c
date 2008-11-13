#include "sflowparser.h"
#include "bufferqueue.h"

extern char** validagents;
extern agentlist_t* agents;
extern buffer_t* buffer_cc_flow;
extern buffer_t* buffer_cc_cntr;


extern int32_t log_level;
extern int32_t cnt_total_f;
extern int32_t cnt_total_c;
extern SFFlowSample** sfbuf;
extern SFCntrSample** scbuf;
extern uint32_t* scnum;
extern uint32_t* sfnum;
extern uint32_t buffer_current_collect;



bool print_parse = false;

uint32_t peekData32(SFDatagram *sample) {
	return ntohl(*(sample->data));
}

uint32_t peekData32_nobswap(SFDatagram *sample) {
	return *(sample->data);
}

uint32_t getData32(SFDatagram *sample) {
	return ntohl(*(sample->data)++);
}

uint32_t getData32_nobswap(SFDatagram *sample) {
	return *(sample->data)++;
}

void skipBytes(SFDatagram *sample, uint32_t skip) {
	int quads = (skip + 3) / 4;
	sample->data += quads;
}

uint64_t getData64(SFDatagram *sample) {
  u_int64_t tmpLo, tmpHi;
  tmpHi = getData32(sample);
  tmpLo = getData32(sample);
  return (tmpHi << 32) + tmpLo;
}

uint32_t getAddress(SFDatagram *sample, SFLAddress *address) {
	address->type = getData32(sample);
	if(address->type == SFLADDRESSTYPE_IP_V4)
		address->address.ip_v4.s_addr = getData32_nobswap(sample);
	else {
		memcpy(&address->address.ip_v6.s6_addr, sample->data, 16);
		skipBytes(sample, 16);
	}
	return address->type;
}

void parseCountersGeneric(SFDatagram* datagram, SFCntrSample* sample)
{
		sample->counter_generic_if_index 			 = getData32(datagram);
		sample->counter_generic_if_type 			 = getData32(datagram);
		sample->counter_generic_if_speed 			 = getData64(datagram);
		sample->counter_generic_if_direction 		 = getData32(datagram);
		sample->counter_generic_if_if_status 		 = getData32(datagram);
		sample->counter_generic_if_in_octets 		 = getData64(datagram);
		sample->counter_generic_if_in_ucast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_in_mcast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_in_bcast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_in_discards 		 = getData32(datagram);
		sample->counter_generic_if_in_errors 		 = getData32(datagram);
		sample->counter_generic_if_in_unknown_proto  = getData32(datagram);
		sample->counter_generic_if_out_octets 		 = getData64(datagram);
		sample->counter_generic_if_out_ucast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_out_mcast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_out_bcast_pkts 	 = getData32(datagram);
		sample->counter_generic_if_out_discards 	 = getData32(datagram);
		sample->counter_generic_if_out_errors 		 = getData32(datagram);
		sample->counter_generic_if_promisc 			 = getData32(datagram);
}

void parseCountersEthernet(SFDatagram* datagram, SFCntrSample* sample)
{
		sample->counter_ethernet_dot3_stats_AlignmentErrors 			= getData32(datagram);
		sample->counter_ethernet_dot3_stats_FCSErrors 					= getData32(datagram);
		sample->counter_ethernet_dot3_stats_SingleCollisionFrames 		= getData32(datagram);
		sample->counter_ethernet_dot3_stats_MultipleCollisionFrames 	= getData32(datagram);
		sample->counter_ethernet_dot3_stats_SQETestErrors 				= getData32(datagram);
		sample->counter_ethernet_dot3_stats_DeferredTransmissions 		= getData32(datagram);
		sample->counter_ethernet_dot3_stats_LateCollisions 				= getData32(datagram);
		sample->counter_ethernet_dot3_stats_ExcessiveCollisions 		= getData32(datagram);
		sample->counter_ethernet_dot3_stats_InternalMacTransmitErrors 	= getData32(datagram);
		sample->counter_ethernet_dot3_stats_CarrierSenseErrors 			= getData32(datagram);
		sample->counter_ethernet_dot3_stats_FrameTooLongs 				= getData32(datagram);
		sample->counter_ethernet_dot3_stats_InternalMacReceiveErrors 	= getData32(datagram);
		sample->counter_ethernet_dot3_stats_SymbolErrors 				= getData32(datagram);
}

void printSampledHeader(SFLSampled_header* hdr)
{
	printf(
			"\t\tSAMPLED HEADER header protocol: %u frame length: %u stripped: %u header length: %u\n",
			hdr->header_protocol,
			hdr->frame_length,
			hdr->stripped,
			hdr->header_length
	);
}

void parseSampledHeader(SFDatagram* datagram, SFFlowSample* sample)
{
	SFLSampled_header hdr;
	memset(&hdr, 0, sizeof(SFLSampled_header));
	hdr.header_protocol = getData32(datagram);
	hdr.frame_length 	= getData32(datagram);
	hdr.stripped 		= getData32(datagram);
	hdr.header_length 	= getData32(datagram);

	if(print_parse) printSampledHeader(&hdr);

	sample->raw_header_protocol 	= hdr.header_protocol;
	sample->raw_header_frame_length = hdr.frame_length;
	sample->raw_header_stripped 	= hdr.stripped;
	sample->raw_header_length 		= hdr.header_length;

	// Allocate dynamic space for the raw header on the heap using the raw_header
	// pointer in the sample structure and copy the raw header
	memset(sample->raw_header, 0, RAW_HEADER_SIZE);
	memcpy(sample->raw_header, datagram->data, hdr.header_length<(uint32_t)RAW_HEADER_SIZE ? hdr.header_length : (uint32_t)RAW_HEADER_SIZE);

	// Since memcpy doesnt update the data pointer, we need to do this manually
	skipBytes(datagram, hdr.header_length); 
}

void printCounterRecordHeader(SFLCounterRecord_hdr* hdr)
{
	printf(
			"\t\t\tRECORD tag: %u:%u length: %u\n",
			hdr->tag >> 12, hdr->tag & 0x00000FFF,
			hdr->length
	);
}

void parseCounterRecordHeader(SFDatagram* datagram, SFCntrSample* sample)
{
	SFLCounterRecord_hdr hdr;
	memset(&hdr, 0, sizeof(SFLCounterRecord_hdr));
	hdr.tag 	= getData32(datagram);
	hdr.length 	= getData32(datagram);

	if(print_parse) printCounterRecordHeader(&hdr);

	if(hdr.tag == SFLCOUNTERS_GENERIC){
		parseCountersGeneric(datagram, sample);
	} else if (hdr.tag == SFLCOUNTERS_ETHERNET){
		parseCountersEthernet(datagram, sample);
	} else {
		// We dont know about this record type yet 
		skipBytes(datagram, hdr.length);
	}
}

void printFlowRecordHeader(SFLFlowRecord_hdr* hdr)
{
	printf(
			"\t\tRECORD tag: %u:%u length: %u\n",
			hdr->tag >> 12, hdr->tag & 0x00000FFF,
			hdr->length
	);
}

void parseFlowRecordHeader(SFDatagram* datagram, SFFlowSample* sample)
{
	SFLFlowRecord_hdr hdr;
	memset(&hdr, 0, sizeof(SFLFlowRecord_hdr));
	hdr.tag = getData32(datagram);
	hdr.length = getData32(datagram);

	if(print_parse) printFlowRecordHeader(&hdr);

	if(hdr.tag == SFLFLOW_HEADER){
		parseSampledHeader(datagram, sample);
	} else {
		// Skip ahead since we cant parse this record
		skipBytes(datagram, hdr.length);
	}
}

void printCounterSample(SFLCounters_sample_expanded* s)
{
	printf(
			"\t\tCNTR SAMPLE seq: %u source id: %u:%u cntr records: %u\n",
			s->sequence_number,
			s->ds_class,
			s->ds_index, 
			s->num_elements
	);
}

void parseCounterSample(SFDatagram* datagram, SFCntrSample* sample, bool expanded)
{
	SFLCounters_sample_expanded s;
	memset(&s, 0, sizeof(SFLCounters_sample_expanded));
	s.sequence_number = getData32(datagram);

	if(expanded){
		s.ds_class = getData32(datagram);
		s.ds_index = getData32(datagram);
	} else {
		int tmp = getData32(datagram);
		s.ds_class = tmp >> 24;
		s.ds_index = tmp & 0x00ffffff;
	}
	s.num_elements = getData32(datagram);
	
	if(print_parse) printCounterSample(&s);

	sample->sample_sequence_number = s.sequence_number;
	sample->sample_source_id_type  = s.ds_class;
	sample->sample_source_id_index = s.ds_index;

	uint32_t i=0;
	for( ; i<s.num_elements ; i++ )
		parseCounterRecordHeader(datagram, sample);
}

void printFlowSample(SFLFlow_sample_expanded* s)
{
	printf(
			"\tFLOW SAMPLE seq: %u source id: %u:%u samplerate: %u sample pool: %u drops: %u input %u:%u output %u:%u flow records: %u\n",
			s->sequence_number,
			s->ds_class,
			s->ds_index, 
			s->sampling_rate,
			s->sample_pool,
			s->drops,
			s->inputFormat,
			s->input,
			s->outputFormat,
			s->output,
			s->num_elements
	);
}

void parseFlowSample(SFDatagram* datagram, SFFlowSample* sample, bool expanded)
{

	SFLFlow_sample_expanded s;
	memset(&s, 0, sizeof(SFLFlow_sample_expanded));
	s.sequence_number 	= getData32(datagram);
	
	if(expanded){
		s.ds_class = getData32(datagram);
		s.ds_index = getData32(datagram);
	} else {
		int32_t tmp = getData32(datagram);
		s.ds_class = tmp >> 24;
		s.ds_index = tmp & 0x00ffffff;
	}

	s.sampling_rate 	= getData32(datagram);
	s.sample_pool 		= getData32(datagram);
	s.drops 			= getData32(datagram);
	
	if(expanded){
		s.inputFormat	= getData32(datagram);
		s.input			= getData32(datagram);
		s.outputFormat	= getData32(datagram);
		s.output		= getData32(datagram);
	} else {
		int tmp = getData32(datagram);
		s.inputFormat	= tmp >> 30;
		s.input			= tmp & 0x3fffffff;
		tmp = getData32(datagram);
		s.outputFormat	= tmp >> 30;
		s.output		= tmp & 0x3fffffff;
	}

	s.num_elements		= getData32(datagram);

	if(print_parse) printFlowSample(&s);

	sample->sample_sequence_number	= s.sequence_number;
	sample->sample_source_id_type 	= s.ds_class;
	sample->sample_source_id_index 	= s.ds_index;
	sample->sample_sampling_rate 	= s.sampling_rate;
	sample->sample_sample_pool		= s.sample_pool;
	sample->sample_drops			= s.drops;
	sample->sample_input_if_format	= s.inputFormat;
	sample->sample_input_if_value	= s.input;
	sample->sample_output_if_format	= s.outputFormat;
	sample->sample_output_if_value	= s.output;

	parseFlowRecordHeader(datagram, sample);
}

void printSampleHeader(SFLSample_hdr* hdr){
	printf("\tSAMPLE ");
	printf("tag: %u:%u ", 	hdr->tag >> 12, hdr->tag & 0x00000FFF);
	printf("length: %u ", 	hdr->length);
	printf("\n");
}

void parseSample(SFDatagram* datagram, SFSample* s_tmpl){

	SFLSample_hdr hdr;
	hdr.tag		= getData32(datagram);
	hdr.length	= getData32(datagram);
	if(print_parse) printSampleHeader(&hdr);
	
	if(hdr.tag == SFLFLOW_SAMPLE || hdr.tag == SFLFLOW_SAMPLE_EXPANDED){

		SFFlowSample* current_buffer = (SFFlowSample*) buffer_cc_flow->data;
		SFFlowSample* s = &current_buffer[buffer_cc_flow->count];
		buffer_cc_flow->count++;

		cnt_total_f++;
		s->timestamp		= s_tmpl->timestamp;
		s->agent_address	= s_tmpl->agent_address;
		s->sub_agent_id		= s_tmpl->sub_agent_id;
		if(hdr.tag == SFLFLOW_SAMPLE_EXPANDED)
			parseFlowSample(datagram, s, true);
		else
			parseFlowSample(datagram, s, false); 
		
	} else if (hdr.tag == SFLCOUNTERS_SAMPLE || hdr.tag == SFLCOUNTERS_SAMPLE_EXPANDED) {

		SFCntrSample* current_buffer = (SFCntrSample*) buffer_cc_cntr->data;
		SFCntrSample* s = &current_buffer[buffer_cc_cntr->count];
		buffer_cc_cntr->count++;

		cnt_total_c++; 
		s->timestamp		= s_tmpl->timestamp;
		s->agent_address	= s_tmpl->agent_address;
		s->sub_agent_id		= s_tmpl->sub_agent_id;

		if(hdr.tag == SFLCOUNTERS_SAMPLE_EXPANDED)
			parseCounterSample(datagram, s, true);
		else
			parseCounterSample(datagram, s, false);

	} else {
		// We dont know what it is, skip ahead to the next sample
		skipBytes(datagram, hdr.length);
	}
}

void printDatagramHeader(const SFLSample_datagram_hdr* hdr){
	printf(
			"DATAGRAM sflow version: %u ip version: %u agent address: %s sub agent: %u sequence number: %u uptime: %u samples: %u\n",
			hdr->sflow_version,
			hdr->datagram_version,
	 		inet_ntoa(hdr->agent_address.address.ip_v4),
			hdr->sub_agent_id,
			hdr->sequence_number,
			hdr->uptime,
			hdr->num_records
		  );
}

void parseDatagram(uint8_t* data, uint32_t n )
{
	// Initialize the pointers in the SFDatagram structure
	// Also set the timestamp this packet was received
	SFDatagram datagram;
	memset(&datagram, 0, sizeof(SFDatagram));
	datagram.data 		= (uint32_t*)data;
	datagram.raw_length = n;
	datagram.raw_sample = data;
	datagram.raw_start 	= data;
	datagram.raw_end 	= data + n;
	datagram.timestamp 	= time(NULL);

	// We extract the information from the sFlow datagram header and populate this structure
	SFLSample_datagram_hdr hdr;
	memset(&hdr, 0, sizeof(SFLSample_datagram_hdr));
	hdr.sflow_version 		= getData32(&datagram);
	hdr.datagram_version 	= peekData32(&datagram);

	// This will get the IP version and then put the address into the datagram structure
	// Note: getAddress also looks at ip versio and moves the pointer for that field,
	// this is why we are only peeking on the datagram_version above
	getAddress(&datagram, &hdr.agent_address);

	// Get the rest of the fields in the datagram header
	hdr.sub_agent_id 		= getData32(&datagram);
	hdr.sequence_number 	= getData32(&datagram);
	hdr.uptime 				= getData32(&datagram);
	hdr.num_records 		= getData32(&datagram);
	
	// We extract the fields we want from our datagram and put it in our template sample
	// This is the information we want in all the samples from the datagram
	SFSample s_template;
	s_template.timestamp 		= datagram.timestamp;
	s_template.agent_address 	= ntohl(hdr.agent_address.address.ip_v4.s_addr);
	s_template.sub_agent_id		= hdr.sub_agent_id;
	
	// Do some stats here (there might be a better way of doing this
	char key[16];
	sprintf(
			key,
			"%i.%i.%i.%i",
			((s_template.agent_address & 0xff000000) >> 24),
			((s_template.agent_address & 0x00ff0000) >> 16),
			((s_template.agent_address & 0x0000ff00) >> 8),
			(s_template.agent_address & 0x000000ff)
		   );

	// Search for this agent in the hash of agents
	//
	
	unsigned int id = cmph_search(h, key, strlen(key));

	
	// If the agent address is valid process the datagram, if not just skip it
	if(key != NULL && strcmp(key, validagents[id]) == 0)
	{
		if(print_parse) printDatagramHeader(&hdr);
		agent_t* agent = agent_get(agents, id);
		agent->datagrams++;
		agent->last_seen = datagram.timestamp;
		uint32_t i;
		for( i=0; i < hdr.num_records; i++ ){
			parseSample(&datagram, &s_template); 				// Populate the sample using the datagram
		}
	} else {
		logmsg(LOGLEVEL_WARNING, "Datagram from unknown agent (%s)", key);
	}
}
