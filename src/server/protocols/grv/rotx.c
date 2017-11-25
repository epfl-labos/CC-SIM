#include "server/protocols/grv/rotx.h"
#include "cluster.h"
#include "common.h"
#include "event.h"
#include "messages/grv.h"
#include "parameters.h"
#include "server/protocols/grv/gst.h"
#include "server/protocols/grv/store.h"
#include <assert.h>
#include <limits.h>

#define NO_SLOT_FOUND UINT_MAX

static DEFINE_PROTOCOL_TIMING_FUNC(process_rotx_request_per_partition_time, "grv");

static unsigned int grv_rotx_state_new(grv_server_state_t *state,
		unsigned int num_keys)
{
	grv_rotx_state_t *rotx = malloc(sizeof(grv_rotx_state_t));
	rotx->dependency_vector = calloc(state->config->cluster->num_replicas,
			sizeof(*rotx->dependency_vector));
	rotx->values = malloc(num_keys * sizeof(*rotx->values));
	rotx->num_values = num_keys;
	rotx->received_values = 0;
	return ptr_array_put(&state->rotx_states, rotx);
}

grv_rotx_state_t *grv_rotx_state_get(grv_server_state_t *state,
		unsigned int id)
{
	return ptr_array_get(&state->rotx_states, id);
}

static void grv_rotx_state_free(grv_server_state_t *state, unsigned int id)
{
	grv_rotx_state_t *rotx = grv_rotx_state_get(state, id);
	free(rotx->values);
	free(rotx->dependency_vector);
	free(rotx);
	ptr_array_set(&state->rotx_states, id, NULL);
}

void grv_process_rotx_request(grv_server_state_t *state,
		grv_rotx_request_t *request)
{
	if (grv_gst_vector_need_update(state, grv_rotx_request_gst_vector(request))) {
		cpu_lock_lock(state->cpu, state->lock_gsv, GRV_ROTX_REQUEST_LOCKED,
				request, request->size);
	} else {
		grv_process_rotx_request_unlocked(state, request);
	}
}

void grv_process_rotx_request_locked(grv_server_state_t *state,
		grv_rotx_request_t *request)
{
	grv_update_gst_vector(state, grv_rotx_request_gst_vector(request));
	cpu_lock_unlock(state->cpu, state->lock_gsv, GRV_ROTX_REQUEST_UNLOCKED,
			request, request->size);
}

void grv_process_rotx_request_unlocked(grv_server_state_t *state,
		grv_rotx_request_t *request)
{
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) (sizeof(grv_rotx_state_t)
							+ sizeof(gr_key) * request->num_keys));
	assert(request->gst_vector_size == state->config->cluster->num_replicas);
	gr_key *keys = grv_rotx_request_keys(request);
	unsigned int rotx_id = grv_rotx_state_new(state, request->num_keys);
	grv_rotx_state_t *rotx = grv_rotx_state_get(state, rotx_id);
	rotx->client_lpid = request->client_lpid;
	gr_tsp snapshot_time = state->clock > request->dependency_time ?
		state->clock : request->dependency_time;
	unsigned int num_replicas = state->config->cluster->num_replicas;

	void send_slice(lpid_t partition_lpid, partition_t partition) {
		(void) partition; // Unused parameter
		unsigned int num_keys = 0;
		for (unsigned int k = 0; k < request->num_keys; ++k) {
			if (grv_lpid_for_key(keys[k], state) == partition_lpid) ++num_keys;
		}
		cpu_add_time(state->cpu, process_rotx_request_per_partition_time());
		if (num_keys == 0) return;

		grv_slice_request_t *slice = grv_slice_request_new(num_keys, num_replicas);
		slice->rotx_id = rotx_id;
		slice->from_lpid = state->config->lpid;
		slice->snapshot_time = snapshot_time;
		grv_copy_gst_vector(grv_slice_request_gst_vector(slice), state);
		gr_key *slice_keys = grv_slice_request_keys(slice);
		unsigned int *slice_key_ids = grv_slice_request_key_ids(slice);
		int slice_key_index = 0;
		for (unsigned int k = 0; k < request->num_keys; ++k) {
			if (grv_lpid_for_key(keys[k], state) == partition_lpid) {
				slice_key_ids[slice_key_index] = k;
				slice_keys[slice_key_index] = keys[k];
				++slice_key_index;
			}
		}
		cpu_add_time(state->cpu, build_struct_per_byte_time()
				* (simtime_t) slice->size);
		server_send(&state->server_state, partition_lpid, GRV_SLICE_REQUEST,
				&slice->message);
		free(slice);
	}
	foreach_partition(state->config->cluster, state->config->replica,
			send_slice);
}

void grv_send_rotx_response(grv_server_state_t *state, unsigned int rotx_id)
{
	grv_rotx_state_t *rotx = grv_rotx_state_get(state, rotx_id);
	grv_rotx_response_t *response = grv_rotx_response_new(
			rotx->num_values, state->config->cluster->num_replicas);
	memcpy(grv_rotx_response_values(response), rotx->values,
			rotx->num_values * sizeof(*rotx->values));
	memcpy(grv_rotx_response_dependency_vector(response), rotx->dependency_vector,
			state->config->cluster->num_replicas * sizeof(*rotx->dependency_vector));
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	server_send(&state->server_state, rotx->client_lpid, GRV_ROTX_RESPONSE,
			&response->message);
	server_stats_counter_inc(&state->server_state, ROTX_REQUESTS);
	free(response);
	grv_rotx_state_free(state, rotx_id);
}
