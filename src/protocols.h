#ifndef protocols_h
#define protocols_h

#include "common.h"

typedef struct client_state client_state_t;
typedef struct server_state server_state_t;
typedef struct client_config client_config_t;

/** .. c:type:: client_functions_t
 *
 *  .. c:member:: void * (*init) (client_state_t *client, const client_config_t *config)
 *
 *      This function must allocate and initialize the state of the protocol
 *      and return a pointer pointing the allocated state.
 *
 *  .. c:member:: void (*get_request) (client_state_t *client, gr_key key)
 *
 *      This function is called when the workload calls :c:func:`client_get_request`.
 *
 *  .. c:member:: void (*put_request) (client_state_t *client, gr_key key, gr_value value)
 *
 *      This function is called when the workload calls :c:func:`client_put_request`.
 *
 *  .. c:member:: void (*rotx_request) (client_state_t *client, gr_key *keys, unsigned int num_keys)
 *
 *      This function is called when the workload calls :c:func:`client_rotx_request`.
 *
 *  .. c:member:: int (*process_event)(client_state_t *client, simtime_t now, unsigned int event_type, void *data, size_t data_size)
 *
 *      This function is called when the client process an event (typically a
 *      message from the network). It must return 1 if the event has been
 *      handled or 0 otherwise.
 */
typedef struct {
	void *(*init)         (client_state_t *client, const client_config_t *config);
	void  (*get_request)  (client_state_t *client, gr_key key);
	void  (*put_request)  (client_state_t *client, gr_key key, gr_value value);
	void  (*rotx_request) (client_state_t *client, gr_key *keys, unsigned int num_keys);
	int   (*process_event)(client_state_t *client, simtime_t now,
			               unsigned int event_type, void *data, size_t data_size);
} client_functions_t;

/** .. c:type:: server_functions_t
 *
 *  .. c:member:: server_state_t *(*allocate_state)(void)
 *
 *  This function must allocate the state of the server (see
 *  :ref:`protocol_server_initialization`).
 *
 *  .. c:member:: void (*init_state) (server_state_t *state)
 *
 *  This function must initialize the protocol-specific server state (see
 *  :ref:`protocol_server_initialization`).
 *
 *  .. c:member:: int (*process_event) (server_state_t *state, unsigned int event_type, void *data, size_t data_size)
 *
 *  This function is called when an event (e.g. message from the network or
 *  event scheduled with :c:func:`server_schedule_self`) needs to be processed.
 *  It must return 1 if the event has been processed or 0 otherwise (e.g. the
 *  event type was unknown).
 */
typedef struct {
	server_state_t *(*allocate_state)(void);
	void (*init_state) (server_state_t *state);
	int (*process_event) (server_state_t *state, unsigned int event_type,
			             void *data, size_t data_size);
} server_functions_t;

typedef struct {
	const char *name;
	client_functions_t *client;
	server_functions_t *server;
} protocol_functions_t;

protocol_functions_t *protocols;

#ifdef protocols_c
#define SCLASS
#else
#define SCLASS extern const
#endif
client_functions_t *client_funcs;
server_functions_t *server_funcs;
#undef SCLASS

#endif
