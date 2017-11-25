#ifndef server_gr_store_h
#define server_gr_store_h

#include "gentle_rain.h"
#include "server/protocols/gr/gr.h"

typedef struct item {
	gr_value value;
	gr_tsp update_time;
	replica_t source_replica;
	struct item *previous_version;
} item_t;

lpid_t gr_lpid_for_key(gr_key key, gr_server_state_t *state);
void gr_garbage_collect(gr_server_state_t *state);
int gr_is_value_visible(gr_server_state_t *state, item_t *item, gr_tsp gst);

int gr_always_visible(gr_server_state_t *state, item_t *item, gr_tsp gst);

void gr_get_value(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, gr_server_state_t *state, gr_key key);

void gr_get_value_at(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, gr_server_state_t *state, gr_key key,
		gr_tsp time,
		int (*is_visible)(gr_server_state_t*, item_t*, gr_tsp)
		);

item_t *gr_put_value(gr_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time,
		replica_t source_replica);

#endif
