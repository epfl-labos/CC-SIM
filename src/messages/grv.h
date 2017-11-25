#ifndef messages_grv_h
#define messages_grv_h

#include "gentle_rain.h"
#include "messages/message.h"

typedef simtime_t gr_gst;

typedef struct grv_get_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	lpid_t proxy_lpid;
	gr_key key;
	unsigned int gst_vector_size; // Array is trailing behind the struct
} grv_get_request_t;

#define grv_get_request_gst_vector(request) \
	((gr_tsp*) (request + 1))

grv_get_request_t *grv_get_request_new(unsigned int gst_vector_size);

typedef struct grv_get_response {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	replica_t source_replica;
	unsigned int gst_vector_size; // Array is trailing behind the struct
	gr_gst gst;
	gr_tsp update_timestamp;
	gr_value value;
} grv_get_response_t;

#define grv_get_response_gst_vector(response) \
	((gr_tsp*) (response + 1))

grv_get_response_t *grv_get_response_new(unsigned int gst_vector_size);

typedef struct grv_put_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	lpid_t proxy_lpid;
	gr_key key;
	gr_value value;
	unsigned int dependency_vector_size; // Array is trailing behind the struct
} grv_put_request_t;

#define grv_put_request_dependency_vector(request) \
	((gr_tsp *) (request + 1))

grv_put_request_t *grv_put_request_new(unsigned int dependency_vector_size);

typedef struct grv_put_response {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	gr_tsp update_time;
	replica_t source_replica;
} grv_put_response_t;

grv_put_response_t *grv_put_response_new(void);

typedef struct grv_replica_update {
	MESSAGE_STRUCT_START;
	gr_key key;
	gr_value value;
	gr_tsp update_time;
	gr_tsp update_time_no_skew;
	replica_t source_replica;
	gr_tsp previous_update_time;
	replica_t previous_source_replica;
	unsigned int dependency_vector_size; // Array is trailing behind the struct
} grv_replica_update_t;

#define grv_replica_update_dependency_vector(update) \
	((gr_tsp*) (update + 1))

grv_replica_update_t *grv_replica_update_new(unsigned int dependency_vector_size);

typedef struct heartbeat {
	MESSAGE_STRUCT_START;
	replica_t replica;
	gr_tsp time;
} grv_heartbeat_t;

grv_heartbeat_t *grv_heartbeat_new(void);

typedef struct grv_gst_from_root {
	MESSAGE_STRUCT_START;
	unsigned int gst_vector_size; // Array is trailing behind the struct
	gr_tsp gst;
} grv_gst_from_root_t;

#define grv_gst_from_root_gst_vector(root_gst) \
	((gr_tsp*) (root_gst + 1))

grv_gst_from_root_t *grv_gst_from_root_new(unsigned int gst_vector_size);

typedef struct grv_lst_from_leaf {
	MESSAGE_STRUCT_START;
	unsigned int leaf_partition_id;
	unsigned int lst_vector_size; // Array is trailing behind the struct
} grv_lst_from_leaf_t;

#define grv_lst_from_leaf_lst_vector(leaf_lst) \
	((gr_tsp*) (leaf_lst + 1))

grv_lst_from_leaf_t *grv_lst_from_leaf_new(unsigned int lst_vector_size);

typedef struct grv_slice_request {
	MESSAGE_STRUCT_START;
	lpid_t from_lpid;
	gr_tsp snapshot_time;
	unsigned int rotx_id;
	unsigned int num_keys; // Keys and key_ids are trailing behind this struct
	unsigned int gst_vector_size; // Vector is trailing behind the key_ids
} grv_slice_request_t;

#define grv_slice_request_keys(request) \
	((gr_key*) (request + 1))
#define grv_slice_request_key_ids(request) \
	((unsigned int*) (grv_slice_request_keys(request) + request->num_keys))
#define grv_slice_request_gst_vector(request) \
	((gr_tsp*) (grv_slice_request_key_ids(request) + request->num_keys))

grv_slice_request_t *grv_slice_request_new(unsigned int num_keys,
		unsigned int gst_vector_size);

typedef struct grv_slice_response {
	MESSAGE_STRUCT_START;
	unsigned int rotx_id;
	unsigned int num_values;
	// Key_ids, values, update times and source replicas are trailing behind
	// this struct
} grv_slice_response_t;

#define grv_slice_response_key_ids(response) \
	((int*) (response + 1))
#define grv_slice_response_values(response) \
	((gr_value*) (grv_slice_response_key_ids(response) + response->num_values))
#define grv_slice_response_update_times(response) \
	((gr_tsp*) (grv_slice_response_values(response) + response->num_values))
#define grv_slice_response_source_replicas(response) \
	((unsigned int*) (grv_slice_response_update_times(response) + response->num_values))

grv_slice_response_t *grv_slice_response_new(unsigned int num_values);

typedef struct grv_rotx_request {
	MESSAGE_STRUCT_START;
	lpid_t client_lpid;
	gr_tsp dependency_time;
	unsigned int num_keys; // The keys (gr_key) follow this struct in the same buffer
	unsigned int gst_vector_size; // Follows the dependency vector
} grv_rotx_request_t;

#define grv_rotx_request_keys(request) \
	((gr_key*) (request + 1))
#define grv_rotx_request_gst_vector(request) \
	((gr_tsp*) (grv_rotx_request_keys(request) \
		+ request->num_keys))

grv_rotx_request_t *grv_rotx_request_new(unsigned int num_keys,
		unsigned int gst_vector_size);

typedef struct grv_rotx_response {
	MESSAGE_STRUCT_START;
	unsigned int num_values;
	// The values (gr_value) follow this struct in the same buffer
	unsigned int dependency_vector_size; // Dependency_vector is trailing behind the values
} grv_rotx_response_t;

#define grv_rotx_response_values(response) \
	((gr_value*) (response + 1))
#define grv_rotx_response_dependency_vector(response) \
	((gr_tsp*) (grv_rotx_response_values(response) + response->num_values))

grv_rotx_response_t *grv_rotx_response_new(unsigned int num_values,
		unsigned int dependency_vector_size);

#endif
