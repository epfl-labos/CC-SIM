#include "server/protocols/gr/store.h"
#include "cluster.h"
#include "common.h"
#include "parameters.h"
#include <assert.h>
#include <ROOT-Sim.h>

static DEFINE_PROTOCOL_TIMING_FUNC(get_value_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(put_value_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(is_value_visible_time, "gr");

lpid_t gr_lpid_for_key(gr_key key, gr_server_state_t *state) {
	partition_t partition = partition_for_key(state->config->cluster, key);
	return cluster_get_lpid(state->config->cluster,
			state->config->replica, partition);
}

void gr_garbage_collect(gr_server_state_t *state)
{
	void garbage_collect_item(gr_key key, void *value) {
		(void) key; // Unused parameter
		item_t *item = (item_t*) value;
		item = item->previous_version; // Always keep the latest version and the previous one.
		while (item != NULL) {
			if (item->update_time < state->gst) {
				item_t *next = item->previous_version;
				item->previous_version = NULL;
				while (next != NULL) {
					item = next;
					next = item->previous_version;
					free(item);
				}
				break;
			}
			item = item->previous_version;
		}
	}
	store_foreach_item(state->store, garbage_collect_item);
}

int gr_is_value_visible(gr_server_state_t *state, item_t *item, gr_tsp gst)
{
	cpu_add_time(state->cpu, is_value_visible_time());
	return item->source_replica == state->config->replica
		|| item->update_time <= gst;
}

int gr_always_visible(gr_server_state_t *state, item_t *item, gr_tsp gst)
{
	(void) state; // Unused parameters
	(void) item;
	(void) gst;
	return 1;
}

void gr_get_value_at(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, gr_server_state_t *state, gr_key key,
		gr_tsp time,
		int (*is_visible)(gr_server_state_t*, item_t*, gr_tsp)
		)
{
	assert(gr_lpid_for_key(key, state) == state->config->lpid);
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

void gr_get_value(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, gr_server_state_t *state, gr_key key)
{
	gr_get_value_at(value_ptr, update_time_ptr, source_replica_ptr,
			state, key, state->gst, gr_is_value_visible);
}

item_t *gr_put_value(gr_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time,
		replica_t source_replica)
{
	assert(gr_lpid_for_key(key, state) == state->config->lpid);
	cpu_add_time(state->cpu, put_value_time());

	item_t *previous_version = store_get(state->store, key);
	item_t *item = malloc(sizeof(item_t));
	item->value = value;
	item->update_time = update_time;
	item->source_replica = source_replica;
	item->previous_version = previous_version;

	// Due to clock skew it is possible for the new value to be older in which
	// case there is a conflict and the old value is ignored
	if (previous_version != NULL
			&& previous_version->update_time > item->update_time) {
		free(item);
		return NULL;
	}
	store_put(state->store, key, item);
	return item;
}
