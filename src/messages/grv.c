#include <ROOT-Sim.h>
#include <stdlib.h>
#include <assert.h>
#include "messages/grv.h"
#include "messages/macros.h"

new_struct_with_1_trailer(grv_get_request, 0, gst_vector_size, gr_tsp,
		sizeof(gr_tsp))

new_struct_with_1_trailer(grv_get_response,
		GR_SIMULATED_VALUE_SIZE - sizeof(gr_value),
		gst_vector_size, gr_tsp, sizeof(gr_tsp))

new_struct_with_1_trailer(grv_put_request,
		GR_SIMULATED_VALUE_SIZE - sizeof(gr_value),
		dependency_vector_size, gr_tsp, sizeof(gr_tsp))

new_struct_simple(grv_put_response, 0)

new_struct_with_1_trailer(grv_replica_update,
		GR_SIMULATED_VALUE_SIZE - sizeof(gr_value) - sizeof(gr_tsp),
		// + simulated value size
		// - dummy value
		// - update_time_no_skew
		dependency_vector_size, gr_tsp, sizeof(gr_tsp))

grv_slice_request_t *grv_slice_request_new(unsigned int num_keys,
		unsigned int gst_vector_size)
{
	size_t size = sizeof(grv_slice_request_t)
		+ num_keys * sizeof(gr_key)
		+ num_keys * sizeof(unsigned int)
		+ gst_vector_size * sizeof(gr_tsp);
	grv_slice_request_t *request =  malloc(size);
	request->num_keys = num_keys;
	request->gst_vector_size = gst_vector_size;
	request->message.size = size;
	request->message.simulated_size = size;
	return request;
}

grv_slice_response_t *grv_slice_response_new(unsigned int num_values)
{
	size_t size =  sizeof(grv_slice_response_t)
		+ num_values * sizeof(unsigned int)
		+ num_values * sizeof(gr_value)
		+ num_values * sizeof(gr_tsp)
		+ num_values * sizeof(replica_t);
	grv_slice_response_t *response = malloc(size);
	response->num_values = num_values;
	response->message.size = size;
	response->message.simulated_size = size
		+ num_values * (GR_SIMULATED_VALUE_SIZE - sizeof(gr_value));
	return response;
}

new_struct_with_2_trailer(grv_rotx_request, 0,
		num_keys, gr_key, sizeof(gr_key),
		gst_vector_size, gr_tsp, sizeof(gr_tsp))

new_struct_with_2_trailer(grv_rotx_response, 0,
		num_values, gr_value, GR_SIMULATED_VALUE_SIZE,
		dependency_vector_size, gr_tsp, sizeof(gr_tsp))

new_struct_simple(grv_heartbeat, 0)

new_struct_with_1_trailer(grv_lst_from_leaf, 0,
		lst_vector_size, gr_tsp, sizeof(gr_tsp))

new_struct_with_1_trailer(grv_gst_from_root, 0,
		gst_vector_size, gr_tsp, sizeof(gr_tsp))
