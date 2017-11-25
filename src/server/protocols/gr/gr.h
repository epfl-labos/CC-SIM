#ifndef server_protocols_gr_gr_h
#define server_protocols_gr_gr_h

#include "cpu/lock.h"
#include "messages/gr.h"
#include "protocols.h"
#include "ptr_array.h"
#include "queue.h"
#include "server/server.h"

typedef struct {
	lpid_t client_lpid;
	gr_tsp dependency_time;
	gr_get_snapshot_request_t *snapshot_request;
	int waiting_gst;
} gr_rotx_state_t;

typedef struct {
	union {
		SERVER_STATE_STRUCT();
		server_state_t server_state;
	};
	gr_tsp *version_vector; // Physical timestamps from each replicas
	gr_tsp gst;
	gr_tsp min_lst;
	int *lst_received;
	unsigned int forwarded_get_id;
	unsigned int forwarded_put_id;
	cpu_lock_id_t lock;
	ptr_array_t get_states;
	ptr_array_t put_states;
	ptr_array_t snapshot_states;
	ptr_array_t rotx_states;
	cpu_lock_id_t *replica_locks;
	queue_t **replica_update_queues;
} gr_server_state_t;

#ifdef server_protocols_gr_gr_c
#define SCLASS
#else
#define SCLASS extern const
#endif
server_functions_t gr_server_funcs;
#undef SCLASS

#endif
