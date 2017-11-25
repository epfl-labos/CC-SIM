#ifndef server_protocols_gr_stats_h
#define server_protocols_gr_stats_h

#include "server/protocols/gr/gr.h"

void gr_stats_get_request(gr_server_state_t *state, gr_key key,
		gr_tsp update_timestamp);
void gr_stats_gst_update(gr_server_state_t *state, gr_tsp old_gst,
		gr_tsp new_gst);

#endif
