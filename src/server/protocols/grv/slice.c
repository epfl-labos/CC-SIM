#include "server/protocols/grv/slice.h"
#include "common.h"
#include "event.h"
#include "messages/grv.h"
#include "parameters.h"
#include "server/protocols/grv/gst.h"
#include "server/protocols/grv/rotx.h"
#include "server/protocols/grv/store.h"
#include <assert.h>

static DEFINE_PROTOCOL_TIMING_FUNC(process_slice_response_per_value_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(is_value_visible_time, "gr");

static int is_value_visible_snapshot(grv_server_state_t *state, item_t *item,
		gr_tsp *time_vector)
{
	for (unsigned int i = 0; i < state->config->cluster->num_replicas; ++i) {
		cpu_add_time(state->cpu, is_value_visible_time());
		if (item->dependency_vector[i] > time_vector[i]) {
			return 0;
		}
	}
	return 1;
}

void grv_process_slice_request(grv_server_state_t *state,
		grv_slice_request_t *request)
{
	cpu_allow_no_time(state->cpu);
	if (request->snapshot_time > state->clock) {
		// Wait until the local clock reaches snapshot_time
		server_schedule_self(&state->server_state,
				request->snapshot_time - state->clock,
				GRV_SLICE_REQUEST, request, request->size);
		return;
	}
	if (grv_gst_vector_need_update(state, grv_slice_request_gst_vector(request))) {
		cpu_lock_lock(state->cpu, state->lock_gsv, GRV_SLICE_REQUEST_LOCKED,
				request, request->size);
	} else {
		grv_process_slice_request_unlocked(state, request);
	}
}
void grv_process_slice_request_locked(grv_server_state_t *state,
		grv_slice_request_t *request)
{
		grv_update_gst_vector(state, grv_slice_request_gst_vector(request));
		cpu_lock_unlock(state->cpu, state->lock_gsv, GRV_SLICE_REQUEST_UNLOCKED,
				request, request->size);
}

void grv_process_slice_request_unlocked(grv_server_state_t *state,
		grv_slice_request_t *request)
{
	unsigned int num_replicas = state->config->cluster->num_replicas;
	gr_tsp *snapshot_vector = malloc(num_replicas * sizeof(gr_tsp));
	grv_copy_gst_vector(snapshot_vector, state);
	snapshot_vector[state->config->replica] = state->clock;
	grv_slice_response_t *response = grv_slice_response_new(request->num_keys);
	response->rotx_id = request->rotx_id;
	gr_key *keys = grv_slice_request_keys(request);
	gr_value *values = grv_slice_response_values(response);
	gr_tsp *update_times = grv_slice_response_update_times(response);
	unsigned int *source_replicas = grv_slice_response_source_replicas(response);
	for (unsigned int i = 0; i < request->num_keys; ++i) {
		grv_get_value_at(&values[i], &update_times[i], &source_replicas[i],
				state, keys[i], snapshot_vector, is_value_visible_snapshot);
	}
	int *key_ids = grv_slice_response_key_ids(response);
	memcpy(key_ids, grv_slice_request_key_ids(request),
			request->num_keys * sizeof(*key_ids));
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	server_send(&state->server_state, request->from_lpid, GRV_SLICE_RESPONSE,
			&response->message);
	free(snapshot_vector);
	free(response);
}

void grv_process_slice_response(grv_server_state_t *state, grv_slice_response_t *slice)
{
	grv_rotx_state_t *rotx = grv_rotx_state_get(state, slice->rotx_id);
	unsigned int *source_replicas = grv_slice_response_source_replicas(slice);
	gr_tsp *update_times = grv_slice_response_update_times(slice);
	int *key_ids = grv_slice_response_key_ids(slice);
	gr_value *values = grv_slice_response_values(slice);
	for (unsigned int i = 0; i < slice->num_values; ++i) {
		unsigned int replica = source_replicas[i];
		set_max(&rotx->dependency_vector[replica], update_times[i]);
		rotx->values[key_ids[i]] = values[i];
	}
	rotx->received_values += slice->num_values;
	cpu_add_time(state->cpu,
			slice->num_values * process_slice_response_per_value_time());
	if (rotx->received_values == rotx->num_values) {
		grv_send_rotx_response(state, slice->rotx_id);
	}
}
