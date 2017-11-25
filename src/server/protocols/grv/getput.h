/* server_getput.{c,h}
 *
 * Implements the logic to handle put and get requests according to the Gentle
 * Rain algorithm.
 */

#ifndef server_protocols_grv_getput_h
#define server_protocols_grv_getput_h

#include "gentle_rain.h"
#include "messages/grv.h"
#include "server/protocols/grv/grv.h"
#include <ROOT-Sim.h>

typedef struct grv_put_state grv_put_state_t;
typedef struct grv_put_state_message grv_put_state_message_t;

void grv_process_get_request(grv_server_state_t *state, grv_get_request_t *request);
void grv_process_forwarded_get_request_locked(grv_server_state_t *state,
		grv_get_request_t *request);
void grv_process_get_request_locked(grv_server_state_t *state,
		grv_get_request_t *request);
void grv_process_get_request_unlocked(grv_server_state_t *state,
		grv_get_request_t *request);
void grv_process_put_request(grv_server_state_t *state, grv_put_request_t *request);
void grv_process_put_request_locked(grv_server_state_t *state,
		grv_put_state_message_t *msg);
void grv_process_put_request_unlocked(grv_server_state_t *state,
		grv_put_state_message_t *msg);
void grv_process_get_response(grv_server_state_t *state,
		grv_get_response_t *response);
void grv_process_put_response(grv_server_state_t *state,
		grv_put_response_t *response);

#endif
