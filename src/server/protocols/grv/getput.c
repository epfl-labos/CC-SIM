#include "server/protocols/gr/getput.h"
#include "common.h"
#include "event.h"
#include "parameters.h"
#include "server/protocols/grv/getput.h"
#include "server/protocols/grv/grv.h"
#include "server/protocols/grv/gst.h"
#include "server/protocols/grv/replication.h"
#include "server/protocols/grv/stats.h"
#include "server/protocols/grv/store.h"
#include "server/stats.h"
#include <assert.h>

// Intentionally shared with gr
static DEFINE_PROTOCOL_TIMING_FUNC(process_get_request_pre_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_put_request_pre_time, "gr");

struct grv_put_state {
	gr_key key;
	gr_value value;
	lpid_t destination;
	grv_put_response_t *response;
	gr_tsp *dependency_vector;
	gr_tsp previous_update_time;
	replica_t previous_source_replica;
	gr_tsp update_time_no_skew;
	int discarded;
};

static unsigned int grv_put_state_new(grv_server_state_t *state) {
	grv_put_state_t *put_state = malloc(sizeof(grv_put_state_t));
	put_state->dependency_vector = malloc(
			state->config->cluster->num_replicas * sizeof(gr_tsp));
	put_state->response = grv_put_response_new();
	return ptr_array_put(&state->put_states, put_state);
}

static grv_put_state_t *grv_put_state_get(grv_server_state_t *state,
		unsigned int id)
{
	return ptr_array_get(&state->put_states, id);
}

static void grv_put_state_free(grv_server_state_t *state, unsigned int id)
{
	grv_put_state_t *put_state = grv_put_state_get(state, id);
	ptr_array_set(&state->put_states, id, NULL);
	free(put_state->response);
	free(put_state->dependency_vector);
	free(put_state);
}
struct grv_put_state_message {
	unsigned int id;
};

static size_t grv_put_state_message_size(grv_put_state_message_t *msg)
{
	(void) msg;
	return sizeof(grv_put_state_message_t);
}

/* Process a get request from a client or another server */
void grv_process_get_request(grv_server_state_t *state, grv_get_request_t *request)
{
	lpid_t key_location = grv_lpid_for_key(request->key, state);
	cpu_add_time(state->cpu, process_get_request_pre_time());
	if (key_location != state->config->lpid) {
		// Request need to be forwarded to the server storing this key
		assert(request->proxy_lpid == NO_PROXY_LPID);
		request->proxy_lpid = state->config->lpid;
		server_send(&state->server_state, key_location, GRV_GET_REQUEST,
				&request->message);
		if (grv_gst_vector_need_update(state, grv_get_request_gst_vector(request))) {
			cpu_lock_lock(state->cpu, state->lock_gsv,
					GRV_FORWARDED_GET_REQUEST_LOCKED, request, request->size);
		}
		grv_update_gst_vector(state, grv_get_request_gst_vector(request));
		server_stats_counter_inc(&state->server_state, state->forwarded_get_id);
		return;
	}

	if (grv_gst_vector_need_update(state, grv_get_request_gst_vector(request))) {
		cpu_lock_lock(state->cpu, state->lock_gsv,
				GRV_GET_REQUEST_LOCKED, request, request->size);
	} else {
		grv_process_get_request_unlocked(state, request);
	}
}

void grv_process_forwarded_get_request_locked(grv_server_state_t *state,
		grv_get_request_t *request)
{
	grv_update_gst_vector(state, grv_get_request_gst_vector(request));
	cpu_lock_unlock(state->cpu, state->lock_gsv, CPU_NO_EVENT, NULL, 0);
}

void grv_process_get_request_locked(grv_server_state_t *state,
		grv_get_request_t *request)
{
	grv_update_gst_vector(state, grv_get_request_gst_vector(request));
	cpu_lock_unlock(state->cpu, state->lock_gsv, GRV_GET_REQUEST_UNLOCKED,
			request, request->size);
}

void grv_process_get_request_unlocked(grv_server_state_t *state,
		grv_get_request_t *request)
{
	grv_get_response_t *response = grv_get_response_new(
			state->config->cluster->num_replicas);
	grv_get_value(&response->value, &response->update_timestamp,
			&response->source_replica, state, request->key);
	grv_copy_gst_vector(grv_get_response_gst_vector(response), state);
	response->client_lpid = request->client_lpid;
	assert(response->source_replica < state->config->cluster->num_replicas);
	lpid_t destination = request->proxy_lpid == NO_PROXY_LPID
			? request->client_lpid : request->proxy_lpid;
	grv_stats_get_request(state, request->key, response->update_timestamp);
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	server_send(&state->server_state, destination, GRV_GET_RESPONSE,
			&response->message);
	free(response);
}

/* Process a put request from a client or another server. */
void grv_process_put_request(grv_server_state_t *state, grv_put_request_t *request)
{
	lpid_t key_location = grv_lpid_for_key(request->key, state);
	cpu_add_time(state->cpu, process_put_request_pre_time());
	if (key_location != state->config->lpid) {
		// The request needs to be forwarded to the server storing this key
		assert(request->proxy_lpid == NO_PROXY_LPID);
		request->proxy_lpid = state->config->lpid;
		server_send(&state->server_state, key_location, GRV_PUT_REQUEST,
				&request->message);
		server_stats_counter_inc(&state->server_state, state->forwarded_put_id);
		return;
	}
	double dependency_time;
	max_array(&dependency_time, grv_put_request_dependency_vector(request),
			request->dependency_vector_size);
	if (dependency_time > state->clock) {
		// Request comes from the future (due to clock skew), wait until the
		// clock increases
		server_schedule_self(&state->server_state,
				dependency_time - state->clock,
				GRV_PUT_REQUEST, request, request->size);
		return;
	}

	grv_put_state_message_t state_message = {
		.id = grv_put_state_new(state)
	};
	grv_put_state_t *put_state = grv_put_state_get(state, state_message.id);
	put_state->key = request->key;
	put_state->value = request->value;
	put_state->destination = request->proxy_lpid == NO_PROXY_LPID
		? request->client_lpid : request->proxy_lpid;
	put_state->response->client_lpid = request->client_lpid;
	put_state->response->source_replica = state->config->replica;;
	memcpy(put_state->dependency_vector,
			grv_put_request_dependency_vector(request),
			state->config->cluster->num_replicas * sizeof(gr_tsp));

	cpu_lock_lock(state->cpu, state->lock_vv, GRV_PUT_REQUEST_LOCKED,
			&state_message, grv_put_state_message_size(&state_message));
}

void grv_process_put_request_locked(grv_server_state_t *state,
		grv_put_state_message_t *msg)
{
	grv_put_state_t *put_state = grv_put_state_get(state, msg->id);
	gr_tsp update_time = state->clock;

	item_t *item = grv_put_value(state, put_state->key, put_state->value,
			update_time, put_state->dependency_vector, state->config->replica);
	state->version_vector[state->config->replica] = update_time;
	put_state->response->update_time = update_time;
	put_state->update_time_no_skew = state->now;

	if (item == NULL) {
		// Update was discarded due to conflict
			put_state->discarded = 1;
	} else {
		put_state->discarded = 0;
		if (item->previous_version != NULL) {
			put_state->previous_update_time = item->previous_version->update_time;
			put_state->previous_source_replica = item->previous_version->source_replica;
		} else {
			put_state->previous_update_time = 0;
			put_state->previous_source_replica = 0;
		}
	}

	cpu_lock_unlock(state->cpu, state->lock_vv, GRV_PUT_REQUEST_UNLOCKED,
			msg, grv_put_state_message_size(msg));
}

void grv_process_put_request_unlocked(grv_server_state_t *state,
		grv_put_state_message_t *msg)
{
	grv_put_state_t *put_state = grv_put_state_get(state, msg->id);
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) put_state->response->simulated_size);
	server_stats_counter_inc(&state->server_state, PUT_REQUESTS);
	server_send(&state->server_state, put_state->destination, GRV_PUT_RESPONSE,
			&put_state->response->message);

	if (!put_state->discarded) {
		grv_replicate_value(state, put_state->key, put_state->value,
				put_state->response->update_time, put_state->dependency_vector,
				state->config->replica, put_state->previous_update_time,
				put_state->previous_source_replica, put_state->update_time_no_skew);
	}

	grv_put_state_free(state, msg->id);
}

void grv_process_get_response(grv_server_state_t *state,
		grv_get_response_t *response)
{
	assert(response->client_lpid != state->config->lpid);
	server_send(&state->server_state, response->client_lpid, GRV_GET_RESPONSE,
			&response->message);
}

void grv_process_put_response(grv_server_state_t *state,
		grv_put_response_t *response)
{
	assert(response->source_replica < state->config->cluster->num_replicas);
	assert(response->client_lpid != state->config->lpid);
	server_send(&state->server_state, response->client_lpid, GRV_PUT_RESPONSE,
			&response->message);
}
