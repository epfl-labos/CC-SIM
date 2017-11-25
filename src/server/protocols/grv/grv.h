#ifndef server_protocols_grv_grv_h
#define server_protocols_grv_grv_h

#include "cpu/lock.h"
#include "protocols.h"
#include "ptr_array.h"
#include "queue.h"
#include "server/server.h"

typedef struct {
	lpid_t client_lpid;
	gr_tsp update_time;
	gr_tsp gst;
	unsigned int num_values;
	gr_value *values;
	unsigned int received_values;
	unsigned int request_id;
	gr_tsp *dependency_vector;
} grv_snapshot_state_t;

typedef struct {
	union {
		SERVER_STATE_STRUCT();
		server_state_t server_state;
	};
	gr_tsp *version_vector; // Physical timestamps from each replicas
	gr_tsp *gst_vector;
	gr_tsp *min_lst_vector;
	int *lst_received;
	unsigned int num_snapshot_states;
	grv_snapshot_state_t **snapshot_states;
	unsigned int forwarded_get_id;
	unsigned int forwarded_put_id;
	cpu_lock_id_t lock_vv;
	cpu_lock_id_t lock_gsv;
	ptr_array_t put_states;
	ptr_array_t rotx_states;
	queue_t **replica_update_queues;
} grv_server_state_t;

#ifdef server_protocols_grv_grv_c
#define SCLASS
#else
#define SCLASS extern const
#endif
server_functions_t grv_server_funcs;
#undef SCLASS

#endif
