/* server_heartbeat.{c,h}
 *
 * Handle heartbeats as part of the Gentle Rain algorithm.
 */

#ifndef server_heartbeat_h
#define server_heartbeat_h

#include "server/protocols/gr/gr.h"
#include "server/server.h"

typedef struct gr_heartbeat gr_heartbeat_t;

void gr_process_heartbeat(gr_server_state_t *state, gr_heartbeat_t *heartbeat);
void gr_process_heartbeat_locked(gr_server_state_t *state, gr_heartbeat_t *heartbeat);
void gr_send_heartbeat(gr_server_state_t *state, int send_time);

#endif
