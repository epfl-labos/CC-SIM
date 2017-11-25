#include <ROOT-Sim.h>
#include <stdlib.h>
#include <assert.h>
#include "messages/gr.h"
#include "messages/macros.h"

new_struct_simple(gr_get_request, 0)

new_struct_simple(gr_get_response, GR_SIMULATED_VALUE_SIZE - sizeof(gr_value))

new_struct_simple(gr_put_request, GR_SIMULATED_VALUE_SIZE - sizeof(gr_value))

new_struct_simple(gr_put_response, 0)

new_struct_simple(gr_replica_update,
		GR_SIMULATED_VALUE_SIZE - sizeof(gr_value) - sizeof(gr_tsp))
		// + simulated value size
		// - dummy value
		// - update_time_no_skew

new_struct_simple(gr_slice_request, 0)

new_struct_simple(gr_slice_response, GR_SIMULATED_VALUE_SIZE - sizeof(gr_value))

new_struct_with_1_trailer(gr_get_snapshot_request, 0,
		num_keys, gr_key, sizeof(gr_key))

new_struct_with_1_trailer(gr_get_snapshot_response, 0,
		num_values, gr_value, GR_SIMULATED_VALUE_SIZE)

new_struct_with_1_trailer(gr_get_rotx_request, 0,
		num_keys, gr_key, sizeof(gr_key))

new_struct_with_1_trailer(gr_get_rotx_response, 0,
		num_values, gr_value, GR_SIMULATED_VALUE_SIZE)

new_struct_simple(gr_gst_from_root, 0)

new_struct_simple(gr_lst_from_leaf, 0)

new_struct_simple(gr_heartbeat, 0)
