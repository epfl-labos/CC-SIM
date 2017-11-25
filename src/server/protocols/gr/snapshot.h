/* server_snapshot.{c,h}
 *
 * Handle snapshot requests according to the Gentle Rain algorithm.
 */

#ifndef server_snapshot_h
#define server_snapshot_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

typedef struct gr_snapshot_state gr_snapshot_state_t;
typedef struct gr_snapshot_state_message gr_snapshot_state_message_t;

struct gr_snapshot_state {
	lpid_t client_lpid;
	gr_tsp time;
	gr_tsp update_time;
	gr_tsp gst;
	unsigned int size;
	gr_key *keys;
	gr_value *values;
	unsigned int received_values;
	unsigned int request_id;
};

gr_snapshot_state_t *gr_snapshot_state_get(gr_server_state_t *state,
		unsigned int id);

void gr_process_get_snapshot_request(gr_server_state_t *state,
		gr_get_snapshot_request_t *request);
void gr_send_snapshot_response(gr_server_state_t *state, unsigned int snapshot_id);
void gr_process_get_snapshot_request_locked(gr_server_state_t *state,
		gr_snapshot_state_message_t *msg);
void gr_process_get_snapshot_request_unlocked(gr_server_state_t *state,
		gr_snapshot_state_message_t *msg);

#endif
