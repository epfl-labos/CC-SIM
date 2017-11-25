#ifndef server_grv_store_h
#define server_grv_store_h

#include "server/protocols/grv/grv.h"

typedef struct item {
	gr_value value;
	gr_tsp update_time;
	gr_tsp *dependency_vector;
	replica_t source_replica;
	struct item *previous_version;
} item_t;

lpid_t grv_lpid_for_key(gr_key key, grv_server_state_t *state);
void grv_garbage_collect(grv_server_state_t *state);
int grv_is_value_visible(grv_server_state_t *state, item_t *item, gr_tsp *gst_vector);

int grv_always_visible(grv_server_state_t *state, item_t *item, gr_tsp *gst_vector);

void grv_get_value(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, grv_server_state_t *state, gr_key key);

void grv_get_value_at(gr_value *value_ptr, gr_tsp *update_time_ptr,
		replica_t *source_replica_ptr, grv_server_state_t *state, gr_key key,
		gr_tsp *time,
		int (*is_visible)(grv_server_state_t*, item_t*, gr_tsp*)
		);

item_t *grv_put_value(grv_server_state_t *state, gr_key key, gr_value value,
		gr_tsp update_time,
		gr_tsp *dependency_vector,
		replica_t source_replica);

#endif
