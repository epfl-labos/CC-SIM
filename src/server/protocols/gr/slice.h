/* server_slice.{c,h}
 *
 * Handle the slice operation as part of the Gentle Rain algorithm.
 */

#ifndef server_slice_h
#define server_slice_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

void gr_process_slice_request(gr_server_state_t *state, gr_slice_request_t *request);
void gr_process_slice_response(gr_server_state_t *state, gr_slice_response_t *slice);
void gr_process_slice_response_locked(gr_server_state_t *state,
		gr_slice_response_t *slice);
void gr_process_slice_response_unlocked(gr_server_state_t *state,
		gr_slice_response_t *slice);
void gr_process_slice_request_locked(gr_server_state_t *state,
		gr_slice_request_t *request);
void gr_process_slice_request_unlocked(gr_server_state_t *state,
		gr_slice_request_t *request);

#endif
