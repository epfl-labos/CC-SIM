/* server_rotx.{c,h}
 *
 * Handle read-only transaction requests according to the Gentle Rain
 * algorithm.
 */

#ifndef server_rotx_h
#define server_rotx_h

#include "messages/grv.h"
#include "server/protocols/grv/grv.h"

typedef struct {
	lpid_t client_lpid;
	gr_tsp *dependency_vector;
	gr_value *values;
	unsigned int num_values;
	unsigned int received_values;
} grv_rotx_state_t;

void grv_send_rotx_snapshot_request(grv_server_state_t *state,
		unsigned int rotx_id);
void grv_process_rotx_request(grv_server_state_t *state,
		grv_rotx_request_t *request);
void grv_process_rotx_request_locked(grv_server_state_t *state,
		grv_rotx_request_t *request);
void grv_process_rotx_request_unlocked(grv_server_state_t *state,
		grv_rotx_request_t *request);
void grv_send_rotx_response(grv_server_state_t *state, unsigned int rotx_id);

grv_rotx_state_t *grv_rotx_state_get(grv_server_state_t *state,
		unsigned int id);

#endif
