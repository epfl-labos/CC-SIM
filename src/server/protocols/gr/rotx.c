#include "server/protocols/gr/rotx.h"
#include "cluster.h"
#include "common.h"
#include "event.h"
#include "parameters.h"
#include "server/protocols/gr/gst.h"
#include "server/protocols/gr/snapshot.h"
#include <assert.h>
#include <limits.h>

static DEFINE_PROTOCOL_TIMING_FUNC(update_gst_per_rotx_time, "gr");

struct gr_rotx_state_message {
	unsigned int id;
};

static unsigned int gr_rotx_state_new(gr_server_state_t *state)
{
	gr_rotx_state_t *rotx = malloc(sizeof(gr_rotx_state_t));
	return ptr_array_put(&state->rotx_states, rotx);
}

static gr_rotx_state_t *gr_rotx_state_get(gr_server_state_t *state, unsigned int id)
{
	return ptr_array_get(&state->rotx_states, id);
}

static void gr_rotx_state_free(gr_server_state_t *state, unsigned int id)
{
	gr_rotx_state_t *rotx = gr_rotx_state_get(state, id);
	free(rotx);
	ptr_array_set(&state->rotx_states, id, NULL);
}

void gr_send_rotx_snapshot_request(gr_server_state_t *state, unsigned int id)
{
	gr_rotx_state_t *rotx = gr_rotx_state_get(state, id);
	server_schedule_self(&state->server_state, 0, GR_GET_SNAPSHOT_REQUEST,
			rotx->snapshot_request,
			rotx->snapshot_request->size);
}

void gr_process_rotx_request(gr_server_state_t *state, gr_get_rotx_request_t *request)
{
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) (sizeof(gr_rotx_state_t)
							+ sizeof(gr_key) * request->num_keys));

	gr_rotx_state_message_t msg = {
		.id = gr_rotx_state_new(state),
	};
	gr_rotx_state_t *rotx = gr_rotx_state_get(state, msg.id);

	rotx->client_lpid = request->client_lpid;
	rotx->dependency_time = request->dependency_time;
	rotx->snapshot_request = gr_get_snapshot_request_new(request->num_keys);
	rotx->snapshot_request->client_lpid = state->config->lpid;
	rotx->snapshot_request->gst = request->gst;
	rotx->snapshot_request->request_id = msg.id;
	memcpy(gr_get_snapshot_request_keys(rotx->snapshot_request),
			gr_get_rotx_request_keys(request),
			request->num_keys * sizeof(gr_key));
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) rotx->snapshot_request->simulated_size);

	if (request->dependency_time <= state->gst) {
		rotx->waiting_gst = 0;
		gr_send_rotx_snapshot_request(state, msg.id);
	} else {
		rotx->waiting_gst = 1;
	}
}

void gr_process_rotx_snapshot_response(gr_server_state_t *state,
		gr_get_snapshot_response_t *snapshot)
{
	unsigned int rotx_id = snapshot->request_id;
	gr_rotx_state_t *rotx = gr_rotx_state_get(state, rotx_id);
	gr_get_rotx_response_t *response =
		gr_get_rotx_response_new(snapshot->num_values);
	memcpy(gr_get_rotx_response_values(response),
			gr_get_snapshot_response_values(snapshot),
			snapshot->num_values * sizeof(gr_value));
	response->gst = snapshot->gst;
	response->update_timestamp = snapshot->update_timestamp;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	server_send(&state->server_state, rotx->client_lpid, GR_ROTX_RESPONSE,
			&response->message);
	server_stats_counter_inc(&state->server_state, ROTX_REQUESTS);
	gr_rotx_state_free(state, rotx_id);
	free(response);
}

static void rotx_gst_update_callback(unsigned int id, void *rotx_, void *data)
{
	gr_server_state_t *state = (gr_server_state_t*) data;
	gr_rotx_state_t *rotx = (gr_rotx_state_t*) rotx_;
	cpu_add_time(state->cpu, update_gst_per_rotx_time());
	if (rotx->waiting_gst && rotx->dependency_time < state->gst) {
		rotx->waiting_gst = 0;
		gr_send_rotx_snapshot_request(state, id);
	}
}

void gr_rotx_on_gst_updated(gr_server_state_t *state)
{
	ptr_array_foreach(&state->rotx_states, rotx_gst_update_callback, state);
}
