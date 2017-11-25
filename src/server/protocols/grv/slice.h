/* server_slice.{c,h}
 *
 * Handle the slice operation as part of the Gentle Rain algorithm.
 */

#ifndef server_slice_h
#define server_slice_h

#include "gentle_rain.h"
#include "messages/grv.h"
#include "server/protocols/grv/grv.h"

void grv_process_slice_request(grv_server_state_t *state, grv_slice_request_t *request);
void grv_process_slice_response(grv_server_state_t *state, grv_slice_response_t *slice);
void grv_process_slice_request_locked(grv_server_state_t *state,
		grv_slice_request_t *request);
void grv_process_slice_request_unlocked(grv_server_state_t *state,
		grv_slice_request_t *request);

#endif
