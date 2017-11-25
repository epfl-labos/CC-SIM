#include "server/protocols/gr/replication.h"
#include "event.h"
#include "parameters.h"
#include "queue.h"
#include "server/protocols/gr/store.h"
#include "server/stats.h"
#include <assert.h>

static DEFINE_PROTOCOL_TIMING_FUNC(process_replica_update_time, "gr");

static void gr_process_replica_update_work(gr_server_state_t *state,
		gr_replica_update_t *update);

void gr_process_replica_update(gr_server_state_t *state, gr_replica_update_t *update)
{
	queue_t *queue = state->replica_update_queues[update->source_replica];
	int process_immediately = queue_is_empty(queue);
	gr_replica_update_t *enqueued_update = malloc(update->size);
	memcpy(enqueued_update, update, update->size);
	queue_enqueue(queue, enqueued_update);
	if (process_immediately) {
		gr_process_replica_update_work(state, update);
	} else {
		cpu_allow_no_time(state->cpu);
	}
}

static void gr_process_replica_update_work(gr_server_state_t *state,
		gr_replica_update_t *update)
{
	assert(update->source_replica != state->config->replica);
	cpu_add_time(state->cpu, process_replica_update_time());

	// Conflict detection
	gr_value value;
	gr_tsp update_time;
	replica_t source_replica;
	gr_get_value_at(&value, &update_time, &source_replica,
			state, update->key, 0, gr_always_visible);
	if (update->previous_update_time != update_time
			|| update->previous_source_replica != source_replica) {
		// There is a conflict, ignore the update if it is older
		if (update->update_time < update_time ||
				(update->update_time == update_time
				 && update->source_replica < source_replica)) {
			assert(update->source_replica != source_replica);
			gr_process_replica_update_unlocked(state, update);
		}
	} else {
		cpu_lock_lock(state->cpu, state->replica_locks[update->source_replica],
				GR_REPLICA_UPDATE_LOCKED, update, update->size);
	}
}

void gr_process_replica_update_locked(gr_server_state_t *state,
		gr_replica_update_t *update)
{

	gr_put_value(state, update->key, update->value, update->update_time, update->source_replica);
	state->version_vector[update->source_replica] = update->update_time;

	server_stats_counter_inc(&state->server_state, REPLICA_UPDATES);
	// Don't use skewed clocks to compute statistics
	simtime_t replication_time = state->now - update->update_time_no_skew;
	assert(replication_time > 0);
	server_stats_array_push(&state->server_state, REPLICATION_TIME,
			replication_time);

	cpu_lock_unlock(state->cpu, state->replica_locks[update->source_replica],
			GR_REPLICA_UPDATE_UNLOCKED, update, update->size);
}

void gr_process_replica_update_unlocked(gr_server_state_t *state,
		gr_replica_update_t *update)
{
	queue_t *queue = state->replica_update_queues[update->source_replica];
	gr_replica_update_t *enqueued_update = queue_dequeue(queue);
	assert(enqueued_update != NULL);
	assert(enqueued_update->key == update->key);
	assert(enqueued_update->value == update->value);
	assert(enqueued_update->update_time == update->update_time);
	assert(enqueued_update->source_replica == update->source_replica);
	free(enqueued_update);

	enqueued_update = queue_peek(queue);
	if (enqueued_update != NULL) {
		gr_process_replica_update_work(state, enqueued_update);
	} else {
		cpu_allow_no_time(state->cpu);
	}
}

void gr_replicate_value(gr_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time, gr_tsp previous_update_time,
		replica_t previous_source_replica, gr_tsp update_time_no_skew)
{
	gr_replica_update_t *update = gr_replica_update_new();
	update->key = key;
	update->value = value;
	update->update_time = update_time;
	update->update_time_no_skew = update_time_no_skew;
	update->source_replica = state->config->replica;
	update->previous_update_time = previous_update_time;
	update->previous_source_replica = previous_source_replica;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) update->simulated_size);

	void send_update(lpid_t replica_lpid, replica_t replica) {
		if (replica == state->config->replica) return;
		server_send(&state->server_state, replica_lpid, GR_REPLICA_UPDATE,
				&update->message);
	}

	foreach_replica(state->config->cluster, state->config->partition,
			send_update);

	free(update);
}
