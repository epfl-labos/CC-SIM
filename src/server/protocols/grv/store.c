#include "server/protocols/grv/store.h"
#include "cluster.h"
#include "common.h"
#include "parameters.h"
#include <assert.h>
#include <ROOT-Sim.h>

static DEFINE_PROTOCOL_TIMING_FUNC(get_value_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(put_value_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(is_value_visible_time, "gr");

lpid_t grv_lpid_for_key(gr_key key, grv_server_state_t *state) {
	partition_t partition = partition_for_key(state->config->cluster, key);
	return cluster_get_lpid(state->config->cluster,
			state->config->replica, partition);
}

void grv_garbage_collect(grv_server_state_t *state)
{
	// FIXME Implement garbage collection in the vector case
	(void) state;
}

int grv_is_value_visible(grv_server_state_t *state, item_t *item, gr_tsp *gst_vector)
{
	cpu_add_time(state->cpu, is_value_visible_time());
	if (item->source_replica == state->config->replica) return 1;
	for (replica_t i = 0; i < state->config->cluster->num_replicas; ++i) {
		cpu_add_time(state->cpu, is_value_visible_time());
		if (i == state->config->replica) continue;
		if (item->dependency_vector[i] > gst_vector[i]) {
			return 0;
		}
	}
	return 1;
}

int grv_always_visible(grv_server_state_t *state, item_t *item, gr_tsp *gst)
{
	(void) state; // Unused parameters
	(void) item;
	(void) gst;
	return 1;
}

void grv_get_value_at(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, grv_server_state_t *state, gr_key key,
		gr_tsp *time,
		int (*is_visible)(grv_server_state_t*, item_t*, gr_tsp*)
		)
{
	assert(grv_lpid_for_key(key, state) == state->config->lpid);
	item_t *item = store_get(state->store, key);
	assert(item == NULL || item->source_replica < state->config->cluster->num_replicas);
	while (item != NULL && !is_visible(state, item, time)) {
		assert(item->source_replica < state->config->cluster->num_replicas);
		item = item->previous_version;
	}
	cpu_add_time(state->cpu, get_value_time());

	if (item == NULL) {
		// FIXME: Properly return the fact that there is no element associated with this key
		if (value_ptr) memset(value_ptr, 0, sizeof(*value_ptr));
		if (update_time_ptr) *update_time_ptr = 0;
		if (source_replica_ptr) *source_replica_ptr = 0;
	} else {
		if (value_ptr) *value_ptr = item->value;
		if (update_time_ptr) *update_time_ptr = item->update_time;
		if (source_replica_ptr) *source_replica_ptr = item->source_replica;
	}
}

void grv_get_value(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, grv_server_state_t *state, gr_key key)
{
	grv_get_value_at(value_ptr, update_time_ptr, source_replica_ptr,
			state, key, state->gst_vector, grv_is_value_visible);
}

item_t *grv_put_value(grv_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time, gr_tsp *dependency_vector, replica_t source_replica)
{
	assert(grv_lpid_for_key(key, state) == state->config->lpid);
	cpu_add_time(state->cpu, put_value_time());

	item_t *previous_version = store_get(state->store, key);
	item_t *item = malloc(sizeof(item_t));
	item->value = value;
	item->update_time = update_time;
	item->source_replica = source_replica;
	item->previous_version = previous_version;

	// Due to clock skew it is possible for the new value to be older in which
	// case there is a conflict and the old values is ignored
	if (previous_version != NULL
			&& previous_version->update_time > item->update_time) {
		free(item);
		return NULL;
	}

	unsigned int num_replicas = state->config->cluster->num_replicas;
	item->dependency_vector = malloc(
			num_replicas * sizeof(*item->dependency_vector));
	memcpy(item->dependency_vector, dependency_vector,
			num_replicas * sizeof(*item->dependency_vector));
	cpu_add_time(state->cpu, (simtime_t) (2 * num_replicas * sizeof(gr_tsp))
			* build_struct_per_byte_time());

	store_put(state->store, key, item);
	return item;
}
