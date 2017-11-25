#ifndef client_workloads_h
#define client_workloads_h

#include "cluster.h"
#include "common.h"
#include "protocols.h"

/** .. c:type:: client_state_t
 *
 * An opaque structure storing the protocol state.
 */
typedef struct client_state client_state_t;

/** .. c:type:: workload_functions_t
 *
 *  Structure containing functions implementing a workload.
 *
 *  .. c:member:: void (*on_init) (void ** data, client_state_t * client_state, cluster_config_t *cluster)
 *
 *     Called when the client is initialized (starts up).
 *
 *  .. c:member:: void (*on_tick)(void**, client_state_t*)
 *
 *     Called at the time when a tick was scheduled with the
 *     :c:func:`client_schedule_tick` function
 *
 *  .. c:member:: void (*on_get_response)(void**, client_state_t*, gr_value)
 *
 *     Called when the client receive a get response from the server.
 *
 *  .. c:member:: void (*on_put_response)(void**, client_state_t*)
 *
 *     Called when the client receive a put response from the server.
 *
 *  .. c:member:: void (*on_rotx_response)(void**, client_state_t*, gr_value*, unsigned int)
 *
 *     Called when the client receive a ROTX response from the server.
 */
typedef struct workload_functions {
	void (*on_init)(void**, client_state_t*, cluster_config_t*);
	void (*on_tick)(void**, client_state_t*);
	void (*on_get_response)(void**, client_state_t*, gr_value);
	void (*on_put_response)(void**, client_state_t*);
	void (*on_rotx_response)(void**, client_state_t*, gr_value*, unsigned int);
} workload_functions_t;

/** .. c:type:: struct workload
 *
 * Workload definition
 *
 *
 * .. c:member:: const char * name
 *
 *    The name of this workload
 *
 *
 * .. c:member:: workload_functions_t * functions
 *
 *    The functions implementing the workload
 */
struct workload {
	const char *name;
	workload_functions_t *functions;
};

#ifdef client_protocols_gr_c
#define SCLASS
#else
#define SCLASS extern const
#endif
struct workload *workloads;
#undef SCLASS

/** .. c:function:: void client_get_request (client_state_t *client, gr_key key)
 *
 *  Perform a get request for the given key.
 */
#define client_get_request client_funcs->get_request

/** .. c:function:: void client_put_request (client_state_t *client, gr_key key, gr_value value)
 *
 *  Perform a put request of the given value to the given key.
 */
#define client_put_request client_funcs->put_request

/** .. c:function:: void client_rotx_request (client_state_t *client, gr_key *keys, unsigned int nem_keys)
 *
 *  Perform a read only transaction with the given keys.
 */
#define client_rotx_request client_funcs->rotx_request

/** .. c:function:: void client_schedule_tick(client_state_t *state, simtime_t delay)
 *
 *  Schedule a call to the :c:member:`workload_functions_t.on_tick` function
 *  in `delay` seconds.
 */
void client_schedule_tick(client_state_t *state, simtime_t delay);

/** .. c:function:: simtime_t client_last_request_duration(client_state_t *state)
 *
 * Return the number of seconds it took for the last request to be answered.
 */
simtime_t client_last_request_duration(client_state_t *state);

/** .. c:function:: simtime_t client_thinking_time(void)
 *
 * Return the client thinking time defined in the configuration
 * (see :ref:`cliennt_thinking_time parameter <timing_client_thinking_time>`).
 */
simtime_t client_thinking_time(void);

#endif
