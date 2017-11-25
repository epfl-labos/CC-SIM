#include "server/protocols/gr/getput.h"
#include "event.h"
#include "parameters.h"
#include "ptr_array.h"
#include "server/protocols/gr/getput.h"
#include "server/protocols/gr/gst.h"
#include "server/protocols/gr/replication.h"
#include "server/protocols/gr/stats.h"
#include "server/protocols/gr/store.h"
#include "server/stats.h"
#include <assert.h>

struct gr_get_state {
	gr_key key;
	gr_gst gst;
	lpid_t destination;
	gr_get_response_t *response;
};

struct gr_get_state_message {
	unsigned int id;
};

struct gr_put_state {
	gr_key key;
	gr_value value;
	gr_put_response_t *response;
	lpid_t destination;
	simtime_t previous_update_time;
	replica_t previous_source_replica;
	gr_tsp update_time_no_skew;
	int discarded;
};

struct gr_put_state_message {
	unsigned int id;
};

static DEFINE_PROTOCOL_TIMING_FUNC(process_get_request_pre_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_put_request_pre_time, "gr");

static unsigned int gr_get_state_new(gr_server_state_t *state)
{
	gr_get_state_t *get_state = malloc(sizeof(gr_get_state_t));
	get_state->response = gr_get_response_new();
	return ptr_array_put(&state->get_states, get_state);
}

static void gr_get_state_free(gr_server_state_t *state, unsigned int id)
{
	gr_get_state_t *get_state = ptr_array_get(&state->get_states, id);
	ptr_array_set(&state->get_states, id, NULL);
	free(get_state->response);
	free(get_state);
}

static gr_get_state_t *gr_get_state_get(gr_server_state_t *state, unsigned int id)
{
	return ptr_array_get(&state->get_states, id);
}

static size_t gr_get_state_message_size(gr_get_state_message_t *msg)
{
	(void) msg;
	return sizeof(gr_get_state_message_t);
}

static unsigned int gr_put_state_new(gr_server_state_t *state)
{
	gr_put_state_t *put_state = malloc(sizeof(gr_put_state_t));
	put_state->response = gr_put_response_new();
	return ptr_array_put(&state->put_states, put_state);
}

static void gr_put_state_free(gr_server_state_t *state, unsigned int id)
{
	gr_put_state_t *put_state = ptr_array_get(&state->put_states, id);
	ptr_array_set(&state->put_states, id, NULL);
	free(put_state->response);
	free(put_state);
}

static gr_put_state_t *gr_put_state_get(gr_server_state_t *state, unsigned int id)
{
	return ptr_array_get(&state->put_states, id);
}

static size_t gr_put_state_message_size(gr_put_state_message_t *msg)
{
	(void) msg;
	return sizeof(gr_put_state_message_t);
}

// Get request from a client or a server
void gr_process_get_request(gr_server_state_t *state, gr_get_request_t *request)
{
	lpid_t key_location = gr_lpid_for_key(request->key, state);
	cpu_add_time(state->cpu, process_get_request_pre_time());
	if (key_location != state->config->lpid) {
		assert(request->proxy_lpid == NO_PROXY_LPID);
		request->proxy_lpid = state->config->lpid;
		server_send(&state->server_state, key_location, GR_GET_REQUEST,
				&request->message);
		server_stats_counter_inc(&state->server_state, state->forwarded_get_id);
		if (gr_gst_need_update(state, request->gst)) {
			cpu_lock_lock(state->cpu, state->lock, GR_FORWARD_GET_REQUEST_LOCKED,
					request, request->size);
		}
		return;
	}
	gr_get_state_message_t state_message = {
		.id = gr_get_state_new(state),
	};
	gr_get_state_t *get_state = gr_get_state_get(state, state_message.id);
	get_state->key = request->key;
	get_state->gst = request->gst;
	get_state->destination = request->proxy_lpid == NO_PROXY_LPID
			? request->client_lpid : request->proxy_lpid;
	gr_get_response_t *response = get_state->response;
	response->client_lpid = request->client_lpid;

	if (gr_gst_need_update(state, get_state->gst)) {
		cpu_lock_lock(state->cpu, state->lock, GR_GET_REQUEST_LOCKED,
				&state_message, gr_get_state_message_size(&state_message));
	} else {
		gr_process_get_request_unlocked(state, &state_message);
	}
}

void gr_process_forward_get_request_locked(gr_server_state_t *state,
		gr_get_request_t *request)
{
	gr_update_gst(state, request->gst);
	cpu_lock_unlock(state->cpu, state->lock, CPU_NO_EVENT, NULL, 0);
}

void gr_process_get_request_locked(gr_server_state_t *state,
		gr_get_state_message_t *state_message)
{
	gr_get_state_t *get_state = gr_get_state_get(state, state_message->id);
	gr_update_gst(state, get_state->gst);
	cpu_lock_unlock(state->cpu, state->lock, GR_GET_REQUEST_UNLOCKED,
			state_message, gr_get_state_message_size(state_message));
}

void gr_process_get_request_unlocked(gr_server_state_t *state,
		gr_get_state_message_t *state_message)
{
	gr_get_state_t *get_state = gr_get_state_get(state, state_message->id);
	gr_get_response_t *response = get_state->response;
	response->gst = state->gst;
	gr_get_value(&(response->value), &(response->update_timestamp), NULL, state,
			get_state->key);
	gr_stats_get_request(state, get_state->key, response->update_timestamp);
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size );
	server_send(&state->server_state, get_state->destination, GR_GET_RESPONSE,
			&response->message);
	gr_get_state_free(state, state_message->id);
}

// Put request from a client or a server
void gr_process_put_request(gr_server_state_t *state, gr_put_request_t *request)
{
	lpid_t key_location = gr_lpid_for_key(request->key, state);
	cpu_add_time(state->cpu, process_put_request_pre_time());
	if (key_location != state->config->lpid) {
		assert(request->proxy_lpid == NO_PROXY_LPID);
		request->proxy_lpid = state->config->lpid;
		server_send(&state->server_state, key_location, GR_PUT_REQUEST,
				&request->message);
		server_stats_counter_inc(&state->server_state, state->forwarded_put_id);
		return;
	}
	if (request->dependency_time > state->clock) {
		// Request comes from the future, wait until the clock increases
		server_schedule_self(&state->server_state,
				request->dependency_time - state->clock,
				GR_PUT_REQUEST, request, request->size);
		return;
	}
	server_stats_counter_inc(&state->server_state, PUT_REQUESTS);
	cpu_add_time(state->cpu,  sizeof(gr_put_response_t) * build_struct_per_byte_time());
	gr_put_state_message_t state_message = {
		.id = gr_put_state_new(state)
	};
	gr_put_state_t *put_state = gr_put_state_get(state, state_message.id);
	put_state->key = request->key;
	put_state->value = request->value;
	put_state->response->client_lpid = request->client_lpid;
	put_state->destination = request->proxy_lpid == NO_PROXY_LPID
		? request->client_lpid : request->proxy_lpid;
	cpu_lock_lock(state->cpu, state->lock, GR_PUT_REQUEST_LOCKED,
			&state_message, gr_put_state_message_size(&state_message));
}

void gr_process_put_request_locked(gr_server_state_t *state,
		gr_put_state_message_t *state_message)
{
	gr_put_state_t *put_state = gr_put_state_get(state, state_message->id);
	state->version_vector[state->config->replica] = state->clock;
	put_state->response->update_timestamp = state->clock;
	put_state->update_time_no_skew = state->now;
	item_t *item = gr_put_value(state, put_state->key, put_state->value,
			state->clock, state->config->replica);

	if (item == NULL) {
		// There was a conflict and this update has been discarded
		put_state->discarded = 1;
	} else {
		put_state->discarded = 0;
		put_state->previous_update_time = item->previous_version == NULL ?
			0 : item->previous_version->update_time;
		put_state->previous_source_replica = item->previous_version == NULL ?
			0 : item->previous_version->source_replica;
	}

	cpu_lock_unlock(state->cpu, state->lock, GR_PUT_REQUEST_UNLOCKED,
			state_message, gr_put_state_message_size(state_message));
}

void gr_process_put_request_unlocked(gr_server_state_t *state,
		gr_put_state_message_t *state_message)
{
	gr_put_state_t *put_state = gr_put_state_get(state, state_message->id);
	server_send(&state->server_state, put_state->destination, GR_PUT_RESPONSE,
			&put_state->response->message);

	// Do not replicate the value if it has been discarded as a result of a conflict
	if (!put_state->discarded) {
		gr_replicate_value(state, put_state->key, put_state->value,
				put_state->response->update_timestamp, put_state->previous_update_time,
				put_state->previous_source_replica, put_state->update_time_no_skew);
	}
	gr_put_state_free(state, state_message->id);
}

// Get response from another server, forward the answer to the client.
void gr_process_get_response(gr_server_state_t *state, gr_get_response_t *response)
{
	assert(response->client_lpid != state->config->lpid);
	server_send(&state->server_state, response->client_lpid, GR_GET_RESPONSE,
			&response->message);
}

// Put response from another server, forward the answer to the client.
void gr_process_put_response(gr_server_state_t *state, gr_put_response_t *response)
{
	assert(response->client_lpid != state->config->lpid);
	server_send(&state->server_state, response->client_lpid, GR_PUT_RESPONSE,
			&response->message);
}
