#ifndef messages_gr_h
#define messages_gr_h

#include "common.h"
#include "gentle_rain.h"
#include "messages/message.h"

typedef simtime_t gr_gst;

typedef struct gr_get_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	lpid_t proxy_lpid;
	gr_key key;
	gr_gst gst;
} gr_get_request_t;

gr_get_request_t *gr_get_request_new(void);

typedef struct gr_get_response {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	gr_gst gst;
	gr_tsp update_timestamp;
	gr_value value;
} gr_get_response_t;

gr_get_response_t *gr_get_response_new(void);

typedef struct gr_put_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	lpid_t proxy_lpid;
	gr_key key;
	gr_value value;
	gr_tsp dependency_time;
} gr_put_request_t;

gr_put_request_t *gr_put_request_new(void);

typedef struct gr_put_response {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	gr_tsp update_timestamp;
} gr_put_response_t;

gr_put_response_t *gr_put_response_new(void);

typedef struct gr_replica_update {
	MESSAGE_STRUCT_START;
	gr_key key;
	gr_value value;
	gr_tsp update_time;
	gr_tsp update_time_no_skew;
	replica_t source_replica;
	gr_tsp previous_update_time;
	replica_t previous_source_replica;
} gr_replica_update_t;

gr_replica_update_t *gr_replica_update_new(void);

typedef struct gr_gst_from_root {
	MESSAGE_STRUCT_START;
	gr_tsp gst;
} gr_gst_from_root_t;

gr_gst_from_root_t *gr_gst_from_root_new(void);

typedef struct gr_heartbeat {
	MESSAGE_STRUCT_START;
	replica_t replica;
	gr_tsp time;
} gr_heartbeat_t;

gr_heartbeat_t *gr_heartbeat_new(void);

typedef struct gr_lst_from_leaf {
	MESSAGE_STRUCT_START;
	unsigned int leaf_partition_id;
	gr_tsp lst;
} gr_lst_from_leaf_t;

gr_lst_from_leaf_t *gr_lst_from_leaf_new(void);

typedef struct gr_get_snapshot_request {
	MESSAGE_STRUCT_START;
	unsigned int request_id;
	lpid_t client_lpid;
	gr_tsp gst;
	unsigned int num_keys;
	// The keys (key) follow this struct in the same buffer
} gr_get_snapshot_request_t;

#define gr_get_snapshot_request_keys(request) \
	((gr_key*) (request + 1))

gr_get_snapshot_request_t *gr_get_snapshot_request_new(unsigned int num_keys);

typedef struct gr_get_snapshot_response {
	MESSAGE_STRUCT_START;
	unsigned int request_id;
	gr_tsp gst;
	gr_tsp update_timestamp;
	unsigned int num_values;
	// The values (value) follow this struct in the same buffer
} gr_get_snapshot_response_t;

#define gr_get_snapshot_response_values(response) \
	((gr_value*) (response + 1))

gr_get_snapshot_response_t *gr_get_snapshot_response_new(unsigned int num_keys);

typedef struct gr_slice_request {
	MESSAGE_STRUCT_START;
	lpid_t from_lpid;
	gr_tsp snapshot_time;
	unsigned int snapshot_id;
	gr_key key;
	unsigned int key_id;
} gr_slice_request_t;

gr_slice_request_t *gr_slice_request_new(void);

typedef struct gr_slice_response {
	MESSAGE_STRUCT_START;
	unsigned int snapshot_id;
	gr_value value;
	gr_tsp update_time;
	gr_tsp gst;
	unsigned int key_id;
} gr_slice_response_t;

gr_slice_response_t *gr_slice_response_new(void);

typedef struct gr_get_rotx_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	gr_tsp dependency_time;
	unsigned int num_keys; // The keys (key) follow this struct in the same buffer
	gr_tsp gst;
} gr_get_rotx_request_t;

#define gr_get_rotx_request_keys(request) \
	((gr_key*) (request + 1))

gr_get_rotx_request_t *gr_get_rotx_request_new(unsigned int num_keys);

typedef struct gr_get_rotx_response {
	MESSAGE_STRUCT_START;
	unsigned int num_values;
	// The values (value) follow this struct in the same buffer
	gr_tsp gst;
	gr_tsp update_timestamp;
} gr_get_rotx_response_t;

#define gr_get_rotx_response_values(response) \
	((gr_value*) (response + 1))

gr_get_rotx_response_t *gr_get_rotx_response_new(unsigned int num_values);

#endif
