#ifndef client_h
#define client_h

#include "client/workloads.h"
#include "cluster.h"
#include "gentle_rain.h"
#include "messages/gr.h"
#include "network.h"
#include "protocols.h"
#include <ROOT-Sim.h>

typedef struct client_state client_state_t;

typedef struct client_config {
	lpid_t lpid;
	replica_t replica;   // The replica_id to which the client connects
	partition_t partition; // If tied_to_partition the client will with this partiton only
	int tied_to_partition;
	cluster_config_t *cluster;
	network_config_t *network;
	workload_functions_t *funcs;
	gr_key (*random_key)(const client_config_t*);
	gr_key (*random_key_on_partition)(const client_config_t*, partition_t);
	double zipf_skew;
} client_config_t;

void client_setup(lpid_t lpid, cluster_config_t *cluster, replica_t replica,
		partition_t partition, network_config_t *network, const char* workload,
		const char *key_distribution, double zipf_skew);

/* Implemented by the protocol-specific client code */

/* For use by the protocol-specific client code */


/** .. c:function:: void client_send(client_state_t *state, lpid_t to_lpid, unsigned int event_type, message_t *message)
 *
 *  Send a network message to another LP.
 */
void client_send(client_state_t *state, lpid_t to_lpid, unsigned int
		event_type, message_t *message);

gr_key client_random_key(client_state_t *state);
gr_key *client_random_keys(client_state_t *state, size_t num_keys);
gr_key client_random_key_on_partition(client_state_t *state, partition_t partition);

/** .. c:function:: void client_begin_request(client_state_t *state)
 *
 *  Indicate that a request starts now. Combine with
 *  :c:func:`client_finish_request` for statistics about requests to be computed.
 */
void client_begin_request(client_state_t *state);

/** .. c:function:: simtime_t client_finish_request(client_state_t *state, int request_type)
 *
 *  Indicate that the client got the response corresponding to the previous
 *  call to :c:func:`client_begin_request`. A new `request_type` is created by
 *  using the :c:func:`client_register_request_type` function.
 */
simtime_t client_finish_request(client_state_t *state, int request_type);

/** .. c:function:: int client_register_request_type(client_state_t *state, const char* name)
 *
 *  Register a new request type to be used with
 *  :c:func:`client_finish_request`. The given `name` will be use to identify
 *  the generated statistics in the simulation output.
 */
int client_register_request_type(client_state_t *state, const char*);

/** .. c:function:: void **client_workload_state_ptr(client_state_t *state)
 *
 *  Return the pointer that the workload implementation uses to manipulate its
 *  state.
 */
void **client_workload_state_ptr(client_state_t *state);

/** .. c:function:: void *client_protocol_state(client_state_t *state)
 *
 *  Return the protocol state of the client. This state is the one the protocol initialized in the :c:member:`client_functions_t.init` function.
 */
void *client_protocol_state(client_state_t *state);

lpid_t lpid_of_any_partition(client_state_t *state);

#endif
