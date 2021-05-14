/*
 * configure.h
 *
 *  Created on: Dec 12, 2017
 *          by: Huu Nghia
 */

#ifndef SRC_LIB_CONFIGURE_H_
#define SRC_LIB_CONFIGURE_H_

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h> //for uint64_t PRIu64
#include <stdbool.h>
#include <errno.h>
#include "lib/limit.h"

#define MMT_USER_AGENT_THRESHOLD 0x20 //32KB

typedef struct multi_thread_conf_struct{
	uint16_t thread_count;
	uint32_t thread_queue_packet_threshold;

}multi_thread_conf_t;


typedef struct file_output_conf_struct{
	bool is_enable;
	char *directory;
	char *filename;
	bool is_sampled;
	uint16_t retained_files_count; //retains the last x sampled files,
								//set to 0 to retain all files
								// ( note that the value of retain-files must be greater than the value of thread_nb + 1)
}file_output_conf_t;

typedef struct internet_service_address_struct{
	char *host_name;
	uint16_t port_number;
}internet_service_address_t;

typedef struct redis_output_conf_struct{
	bool is_enable;
	internet_service_address_t host;
	char *channel_name;
}redis_output_conf_t;

typedef struct kafka_output_conf_struct{
	bool is_enable;
	internet_service_address_t host;
	//further setting for kafka connection, such as, TLS certificate, cipher type, ...
	char *topic_name;
}kafka_output_conf_t;

typedef struct mongodb_output_conf_struct{
	bool is_enable;
	internet_service_address_t host;
	char *database_name;
	char *collection_name;
	uint32_t limit_size; //limit size of storage collection
	//further setting for kafka connection, such as, TLS certificate, cipher type, ...
}mongodb_output_conf_t;

typedef struct input_source_conf_struct{

	enum {ONLINE_ANALYSIS, OFFLINE_ANALYSIS} input_mode;
	enum {DPDK_CAPTURE, PCAP_CAPTURE} capture_mode;

	//input source for PCAP online mode (interface name) and for offline mode (pcap name), however for DPDK its interface port number
	char *input_source;
	uint16_t snap_len;
}input_source_conf_t;


typedef enum {
	CONF_OUTPUT_CHANNEL_NONE    = 0,
	CONF_OUTPUT_CHANNEL_FILE    = 1,
	CONF_OUTPUT_CHANNEL_REDIS   = 2,
	CONF_OUTPUT_CHANNEL_KAFKA   = 4,
	CONF_OUTPUT_CHANNEL_MONGODB = 8,
	CONF_OUTPUT_CHANNEL_SOCKET  = 16,
	CONF_OUTPUT_CHANNEL_ALL     = CONF_OUTPUT_CHANNEL_FILE  | CONF_OUTPUT_CHANNEL_REDIS
								| CONF_OUTPUT_CHANNEL_KAFKA | CONF_OUTPUT_CHANNEL_MONGODB
								| CONF_OUTPUT_CHANNEL_SOCKET
}output_channel_conf_t;

#define IS_ENABLE_OUTPUT_TO( name, channels ) ( channels & CONF_OUTPUT_CHANNEL_ ##name )
#define IS_ENABLE_OUTPUT_TO_ALL_CHANNELS( channles ) ( channels & CONF_OUTPUT_CHANNEL_ALL   )
#define IS_ENABLE_OUTPUT_TO_ONE_CHANNEL(  channles ) ( channels | CONF_OUTPUT_CHANNEL_ALL   )
#define IS_DISABLE_OUTPUT( channels )                ( channels == CONF_OUTPUT_CHANNEL_NONE )

typedef struct dynamic_config_conf_struct{
	bool is_enable;
	char *descriptor;
}dynamic_config_conf_t;

typedef enum{
	CONF_IP_ENCAPSULATION_INDEX_FIRST   = 1,
	CONF_IP_ENCAPSULATION_INDEX_SECOND     ,
	CONF_IP_ENCAPSULATION_INDEX_THIRD      ,
	CONF_IP_ENCAPSULATION_INDEX_FOURTH     ,
	CONF_IP_ENCAPSULATION_INDEX_FIFTH      ,
	CONF_IP_ENCAPSULATION_INDEX_SIXTH      ,
	CONF_IP_ENCAPSULATION_INDEX_SEVENTH    ,
	CONF_IP_ENCAPSULATION_INDEX_EIGHTH     ,
	CONF_IP_ENCAPSULATION_INDEX_NINTH      ,
	CONF_IP_ENCAPSULATION_INDEX_TENTH      ,
	CONF_IP_ENCAPSULATION_INDEX_ELEVENTH   ,
	CONF_IP_ENCAPSULATION_INDEX_TWELFTH    ,
	CONF_IP_ENCAPSULATION_INDEX_THIRTEENTH ,
	CONF_IP_ENCAPSULATION_INDEX_FOURTEENTH ,
	CONF_IP_ENCAPSULATION_INDEX_FIFTEENTH  ,
	CONF_IP_ENCAPSULATION_INDEX_LAST   /*currently, MMT supports maximally 16 proto in hierarchies*/
}conf_ip_encapsulation_index_t;

typedef enum{
	CONF_RTT_BASE_SENDER        = 1,
	CONF_RTT_BASE_CAPTOR        = 2,
	CONF_RTT_BASE_PREFER_SENDER = 3
}conf_rtt_base_t;

typedef struct security_conf_struct{
	bool is_enable;
	bool is_report_rule_description;
//	enum{
//		SEC_IGNORE_FLOW_NONE,     //do not ignore any packets
//		SEC_IGNORE_FLOW_SAME_RULE,//ignore the verification of a rule on the rest of a flow if the rule has been satisfied by the flow
//		SEC_IGNORE_FLOW_ALL_RULE  //ignore the verification of all rules on the rest of a flow if a rule has been satisfied by the flow
//	}
	bool ignore_remain_flow;
	uint16_t threads_size;
	char *excluded_rules;
	char *rules_mask;
	output_channel_conf_t output_channels;
	struct{
		uint32_t input_max_message_size;
		uint32_t security_max_instances;
		uint32_t security_smp_ring_size;
	}lib;
	//indicate which IP will be analyzed in case of IP-in-IP
	conf_ip_encapsulation_index_t ip_encapsulation_index;
}security_conf_t;


typedef struct system_stats_conf_struct{
	bool is_enable;
	uint16_t frequency; //time-interval for reporting
	output_channel_conf_t output_channels;
}system_stats_conf_t;


typedef struct pcap_dump_conf_struct{
	bool is_enable;
	char *directory;
	uint16_t frequency;

	uint16_t protocols_size;
	char **protocols;
	uint16_t retained_files_count; //retains the last x sampled files,
									//set to 0 to retain all files
									// ( note that the value of retain-files must be greater than the value of thread_nb + 1)
	uint16_t snap_len;
}pcap_dump_conf_t;

typedef enum {
	ACTION_FORWARD,
	ACTION_DROP
}forward_action_t;

typedef struct forward_packet_target_conf_struct{
	enum{ FORWARD_PACKET_PROTO_SCTP } protocol;
	char * host;
	uint16_t port;
}forward_packet_target_conf_t;

typedef struct forward_packet_conf_struct{
	bool is_enable;
	char *output_nic;
	uint16_t snap_len;
	uint16_t promisc;
	uint32_t nb_copies;
	forward_action_t default_action;

	forward_packet_target_conf_t *targets;
	uint16_t target_size;
}forward_packet_conf_t;

typedef struct reconstruct_data_conf_struct{
	bool is_enable;
	char *directory; //indicates the folder where the output file is created
	output_channel_conf_t output_channels;
}reconstruct_data_conf_t;


typedef struct dpi_protocol_attribute_struct{
	char* proto_name;
	char* attribute_name;
}dpi_protocol_attribute_t;


typedef struct socket_output_conf_struct{
	bool is_enable;
	enum{ SOCKET_TYPE_ANY = 0, SOCKET_TYPE_UNIX, SOCKET_TYPE_INTERNET } socket_type;
	//descriptor of UNIX socket if used
	char *unix_socket_descriptor;

	internet_service_address_t internet_socket;
}socket_output_conf_t;

typedef struct micro_flow_conf_struct{
	bool is_enable;
	uint32_t packet_threshold;
	uint32_t byte_threshold;
	uint32_t report_packets_count;
	uint32_t report_bytes_count;
	uint32_t report_flows_count;
	output_channel_conf_t output_channels;
}micro_flow_conf_t;

typedef struct session_timeout_conf_struct{
	uint32_t default_session_timeout;
	uint32_t long_session_timeout;
	uint32_t short_session_timeout;
	uint32_t live_session_timeout;
}session_timeout_conf_t;

typedef struct event_report_conf_struct{
	bool is_enable;
	char *title;
	dpi_protocol_attribute_t *event;
	uint16_t attributes_size;
	dpi_protocol_attribute_t *attributes;
	output_channel_conf_t output_channels;
}event_report_conf_t;


typedef struct session_report_conf_struct{
	bool is_enable;
	output_channel_conf_t output_channels;
	bool is_ftp;
	bool is_http;
	bool is_ssl;
	bool is_rtp;
	bool is_gtp;
	conf_rtt_base_t rtt_base; //timestamp's origin will be used to calculate RTT, e.g., CAPTURE_TS, SENDER_TS
}session_report_conf_t;

typedef struct radius_report_conf_struct{
	bool is_enable;
	uint16_t message_code;
	output_channel_conf_t output_channels;
}radius_report_conf_t;


typedef struct output_conf_struct{
	bool is_enable;
	uint32_t cache_max;
	uint16_t cache_period;

	file_output_conf_t  *file;
	redis_output_conf_t *redis;
	kafka_output_conf_t *kafka;
	mongodb_output_conf_t *mongodb;
	socket_output_conf_t *socket;
	enum {OUTPUT_FORMAT_CSV, OUTPUT_FORMAT_JSON} format;
}output_conf_t;

/**
 * Configuration of MMT-Probe
 */
typedef struct probe_conf_struct{

	bool is_enable_ip_fragmentation_report;
	bool is_enable_proto_no_session_report; //report informations about fragmentation
	bool is_enable_ip_defragmentation;      //perform de-fragmentation
	bool is_enable_tcp_reassembly;

	uint32_t stack_type; //dpi stack type
	uint32_t probe_id;

	output_conf_t outputs;

	multi_thread_conf_t *thread;

	session_timeout_conf_t *session_timeout;
	dynamic_config_conf_t *dynamic_conf;
	forward_packet_conf_t *forward_packet;

	struct report_conf_struct{
		security_conf_t *security;
		system_stats_conf_t *cpu_mem;
		file_output_conf_t  *behaviour;
		micro_flow_conf_t *microflow;
		session_report_conf_t *session;

		uint16_t events_size;
		event_report_conf_t *events;

		pcap_dump_conf_t *pcap_dump;
		radius_report_conf_t *radius;
	}reports;

	struct reconstruct_data_struct{
		reconstruct_data_conf_t *http;
		reconstruct_data_conf_t *ftp;
	}reconstructions;


	uint16_t stat_period;

	char *license_file;

	input_source_conf_t *input;
	char *dpdk_options;
}probe_conf_t;


/**
 * Load configuration from a file
 * @param filename
 * @return
 */
probe_conf_t* conf_load_from_file( const char* filename );

/**
 * Free all memory allocated by @load_configuration_from_file
 * @param
 */
void conf_release( probe_conf_t * );

/**
 * Parse a string to get output_channel
 * @param string contains set of output channels separated by comma. For example: "redis, kafka, file"
 * @return
 */
output_channel_conf_t conf_parse_output_channel( const char *string );

/**
 * Split a string into an array of string segments with separator is comma
 * @param string
 * @param proto_lst is a pointer to a the result array that will be created by the function.
 * @return number of segments in proto_lst
 * @note: user needs to free the segments in proto_lst and also proto_lst after using them
 */
size_t conf_parse_list( const char *string, char ***proto_lst );

/**
 * Validate a configuration to avoid any conflict among its parameters, such as,
 * if you enable http reconstruction, then tcp reassembly must be enabled also.
 * @param conf
 * @return
 */
int conf_validate( probe_conf_t *conf );

bool conf_parse_input_mode( int *result, const char *string );
bool conf_parse_rtt_base(int *result, const char *value);
//bool conf_parse_security_ignore_mode( int *result, const char *string );
#endif /* SRC_LIB_CONFIGURE_H_ */
