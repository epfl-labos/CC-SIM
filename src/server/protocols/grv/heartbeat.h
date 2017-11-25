/* server_heartbeat.{c,h}
 *
 * Handle heartbeats as part of the Gentle Rain algorithm.
 */

#ifndef server_heartbeat_h
#define server_heartbeat_h

#include "messages/grv.h"
#include "server/protocols/grv/grv.h"
#include "server/server.h"

void grv_process_heartbeat(grv_server_state_t *state,
		grv_heartbeat_t *heartbeat);
void grv_process_heartbeat_locked(grv_server_state_t *state,
		grv_heartbeat_t *heartbeat);
void grv_send_heartbeat(grv_server_state_t *state, int send_time);

#endif
