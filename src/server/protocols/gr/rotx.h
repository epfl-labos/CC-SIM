/* server_rotx.{c,h}
 *
 * Handle read-only transaction requests according to the Gentle Rain
 * algorithm.
 */

#ifndef server_rotx_h
#define server_rotx_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

typedef struct gr_rotx_state_message gr_rotx_state_message_t;

void gr_send_rotx_snapshot_request(gr_server_state_t *state, unsigned int rotx_id);
void gr_process_rotx_request(gr_server_state_t *state, gr_get_rotx_request_t *request);
void gr_process_rotx_snapshot_response(gr_server_state_t *state, gr_get_snapshot_response_t *snapshot);
void gr_send_rotx_response(gr_server_state_t *state, unsigned int rotx_id);
void gr_rotx_on_gst_updated(gr_server_state_t *state);

#endif
