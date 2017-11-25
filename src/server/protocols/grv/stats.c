#include "server/protocols/grv/stats.h"
#include "common.h"
#include "messages/grv.h"
#include "parameters.h"
#include "server/protocols/grv/store.h"
#include "server/stats.h"

void grv_stats_get_request(grv_server_state_t *state, gr_key key,
		gr_tsp update_timestamp)
{
	if (state->now < app_params.ignore_initial_seconds) return;
	server_stats_counter_inc(&state->server_state, GET_REQUESTS);
	item_t *latest_item = store_get(state->store, key);
	simtime_t latest_update = latest_item ? latest_item->update_time : 0;
	server_stats_array_push(&state->server_state, VALUE_STALENESS,
			latest_update - update_timestamp);
}

void grv_stats_gst_update(grv_server_state_t *state, gr_tsp *old_gst_vector,
		gr_tsp *new_gst_vector)
{
	if (state->now < app_params.ignore_initial_seconds) return;
	simtime_t saved_time = cpu_elapsed_time(state->cpu);
	void process_item(gr_key key, void *_item) {
		(void) key; // Unused parameter
		item_t *item = (item_t*) _item;
		while (item != NULL && !grv_is_value_visible(state, item, old_gst_vector))
		{
			if (grv_is_value_visible(state, item, new_gst_vector)) {
				server_stats_array_push(&state->server_state,
						VISIBILITY_LATENCY,
						state->now - item->update_time);
			}
			item = item->previous_version;
		}
	}
	store_foreach_item(state->store, process_item);
	cpu_set_elapsed_time(state->cpu, saved_time);
}
