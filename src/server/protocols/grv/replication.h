/* server_replication.{c,h}
 *
 * Replication of updates according to the Gentle Rain algorithm.
 */

#ifndef server_replication_h
#define server_replication_h

#include "gentle_rain.h"
#include "messages/grv.h"
#include "server/protocols/grv/grv.h"
#include "server/protocols/grv/store.h"

void grv_replicate_value(grv_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time, gr_tsp *dependency_vector, replica_t source_replica,
		gr_tsp previous_update_time, replica_t previous_source_replica,
		gr_tsp update_time_no_skew);
void grv_process_replica_update(grv_server_state_t *state, grv_replica_update_t *update);
void grv_process_replica_update_vv_locked(grv_server_state_t *state,
		grv_replica_update_t *update);
void grv_process_replica_update_vv_unlocked(grv_server_state_t *state,
		grv_replica_update_t *update);

#endif
