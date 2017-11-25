#include "server/protocols/gr/snapshot.h"
#include "common.h"
#include "event.h"
#include "parameters.h"
#include "server/protocols/gr/gst.h"
#include "server/protocols/gr/rotx.h"
#include "server/protocols/gr/store.h"
#include <assert.h>
#include <limits.h>

struct gr_snapshot_state_message{
	unsigned int id;
};

static unsigned int gr_snapshot_state_new(gr_server_state_t *state,
		unsigned int size)
{
	gr_snapshot_state_t *snapshot_state = malloc(sizeof(gr_snapshot_state_t));
	snapshot_state->size = size;
	snapshot_state->keys = malloc(size * sizeof(gr_key));
	snapshot_state->values = malloc(size * sizeof(gr_value));
	return ptr_array_put(&state->snapshot_states, snapshot_state);
}

gr_snapshot_state_t *gr_snapshot_state_get(gr_server_state_t *state,
		unsigned int id)
{
	return ptr_array_get(&state->snapshot_states, id);
}

static void gr_snapshot_state_free(gr_server_state_t *state, unsigned int id)
{
	gr_snapshot_state_t *snapshot_state = ptr_array_get(&state->snapshot_states, id);
	free(snapshot_state->keys);
	free(snapshot_state->values);
	free(snapshot_state);
	ptr_array_set(&state->snapshot_states, id, NULL);
}

static size_t gr_snapshot_state_simulated_size(gr_snapshot_state_t *snapshot)
{
	return sizeof(gr_snapshot_state_t) + snapshot->size *
		(sizeof(gr_key) + GR_SIMULATED_VALUE_SIZE);
}

static size_t gr_snapshot_state_message_size(gr_snapshot_state_message_t *msg)
{
	(void) msg;
	return sizeof(gr_snapshot_state_message_t);
}

void gr_process_get_snapshot_request(gr_server_state_t *state,
		gr_get_snapshot_request_t *request)
{
	unsigned int snapshot_id = gr_snapshot_state_new(state, request->num_keys);
	gr_snapshot_state_t *snapshot = gr_snapshot_state_get(state, snapshot_id);
	snapshot->client_lpid = request->client_lpid;
	snapshot->request_id = request->request_id;
	snapshot->update_time = 0;
	snapshot->gst = request->gst;
	memcpy(snapshot->keys, gr_get_snapshot_request_keys(request),
			request->num_keys * sizeof(gr_key));

	snapshot->received_values = 0;
	cpu_add_time(state->cpu, build_struct_per_byte_time() *
		(simtime_t) gr_snapshot_state_simulated_size(snapshot));

	gr_snapshot_state_message_t msg = {
		.id = snapshot_id,
	};
	cpu_lock_lock(state->cpu, state->lock, GR_GET_SNAPSHOT_REQUEST_LOCKED,
			&msg, gr_snapshot_state_message_size(&msg));
}

void gr_process_get_snapshot_request_locked(gr_server_state_t *state,
		gr_snapshot_state_message_t *msg)
{
	gr_snapshot_state_t *snapshot = gr_snapshot_state_get(state, msg->id);
	gr_update_gst(state, snapshot->gst);
	snapshot->gst = 0;
	snapshot->time = state->gst;
	cpu_lock_unlock(state->cpu, state->lock, GR_GET_SNAPSHOT_REQUEST_UNLOCKED,
			msg, gr_snapshot_state_message_size(msg));
}

void gr_process_get_snapshot_request_unlocked(gr_server_state_t *state,
		gr_snapshot_state_message_t *msg)
{
	gr_snapshot_state_t *snapshot = gr_snapshot_state_get(state, msg->id);
	gr_slice_request_t *slice = gr_slice_request_new();
	slice->from_lpid = state->config->lpid;
	slice->snapshot_time = snapshot->time;
	slice->snapshot_id = msg->id;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) slice->simulated_size);
	for (unsigned int i = 0; i < snapshot->size; ++i) {
		slice->key_id = i;
		slice->key = snapshot->keys[i];
		server_send(&state->server_state, gr_lpid_for_key(slice->key, state),
				GR_SLICE_REQUEST, &slice->message);
	}
}

void gr_send_snapshot_response(gr_server_state_t *state, unsigned int snapshot_id)
{
	gr_snapshot_state_t *snapshot = gr_snapshot_state_get(state, snapshot_id);
	gr_get_snapshot_response_t *response =
		gr_get_snapshot_response_new(snapshot->size);
	response->gst = snapshot->gst;
	response->update_timestamp = snapshot->update_time;
	response->request_id = snapshot->request_id;
	memcpy(gr_get_snapshot_response_values(response), snapshot->values,
			snapshot->size * sizeof(gr_value));
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	if (state->config->lpid == snapshot->client_lpid) {
		gr_process_rotx_snapshot_response(state, response);
	} else {
		server_send(&state->server_state, snapshot->client_lpid,
				GR_GET_SNAPSHOT_RESPONSE, &response->message);
	}
	free(response);
	gr_snapshot_state_free(state, snapshot_id);
}
