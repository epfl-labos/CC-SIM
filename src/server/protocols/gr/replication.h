/* server_replication.{c,h}
 *
 * Replication of updates according to the Gentle Rain algorithm.
 */

#ifndef server_replication_h
#define server_replication_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

void gr_replicate_value(gr_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time, gr_tsp previous_update_time,
		replica_t previous_source_replica, gr_tsp update_time_no_skew_);
void gr_process_replica_update(gr_server_state_t *state, gr_replica_update_t *update);
void gr_process_replica_update_locked(gr_server_state_t *state,
		gr_replica_update_t *update);
void gr_process_replica_update_unlocked(gr_server_state_t *state,
		gr_replica_update_t *update);

#endif
