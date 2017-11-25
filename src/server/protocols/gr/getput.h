/* server_getput.{c,h}
 *
 * Implements the logic to handle put and get requests according to the Gentle
 * Rain algorithm.
 */

#ifndef server_protocols_gr_getput_h
#define server_protocols_gr_getput_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

typedef struct gr_get_state gr_get_state_t;
typedef struct gr_get_state_message gr_get_state_message_t;
typedef struct gr_put_state gr_put_state_t;
typedef struct gr_put_state_message gr_put_state_message_t;


void gr_process_get_request(gr_server_state_t *state, gr_get_request_t *request);
void gr_process_forward_get_request_locked(gr_server_state_t *state,
		gr_get_request_t *request);
void gr_process_get_request_locked(gr_server_state_t *state,
		gr_get_state_message_t *state_message);
void gr_process_get_request_unlocked(gr_server_state_t *state,
		gr_get_state_message_t *state_message);
void gr_process_put_request(gr_server_state_t *state, gr_put_request_t *request);
void gr_process_put_request_locked(gr_server_state_t *state,
		gr_put_state_message_t *_state_message);
void gr_process_put_request_unlocked(gr_server_state_t *state,
		gr_put_state_message_t *state_message);
void gr_process_get_response(gr_server_state_t *state, gr_get_response_t *response);
void gr_process_put_response(gr_server_state_t *state, gr_put_response_t *response);

#endif
