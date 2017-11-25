#ifndef server_protocols_grv_stats_h
#define server_protocols_grv_stats_h

#include "server/protocols/grv/grv.h"

void grv_stats_get_request(grv_server_state_t *state, gr_key key,
		gr_tsp update_timestamp);
void grv_stats_gst_update(grv_server_state_t *state, gr_tsp *old_gst_vector,
		gr_tsp *new_gst_vector);

#endif
