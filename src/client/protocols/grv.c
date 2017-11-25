#define client_protocols_grv_c
#include "client/protocols/grv.h"
#include "client/client.h"
#include "client/protocols/gr_common.h"
#include "common.h"
#include "event.h"
#include "messages/grv.h"
#include "protocols.h"
#include <assert.h>

typedef struct {
	const client_config_t *config;
	gr_tsp *dependency_vector;
	gr_tsp *gst_vector;
} client_state_grv_t;

int get_request_type, put_request_type, rotx_request_type;

static void *grv_client_init(client_state_t *client, const client_config_t *config)
{
	get_request_type = client_register_request_type(client, "get");
	put_request_type = client_register_request_type(client, "put");
	rotx_request_type = client_register_request_type(client, "rotx");
	client_state_grv_t *state = malloc(sizeof(client_state_grv_t));
	state->config = config;
	state->dependency_vector = calloc(config->cluster->num_replicas,
			sizeof(*state->dependency_vector));
	state->gst_vector = calloc(config->cluster->num_replicas,
			sizeof(*state->gst_vector));
	return state;
}

static void update_dependency_vector(client_state_grv_t *state, gr_tsp *dependency_vector)
{
	for (unsigned int i = 0; i < state->config->cluster->num_replicas; ++i) {
		set_max(&state->dependency_vector[i], dependency_vector[i]);
	}
}

static void update_times(client_state_grv_t *state, gr_tsp *gst_vector,
		gr_tsp dependency_time, replica_t replica)
{
	if (gst_vector != NULL) {
		for (unsigned int i = 0; i < state->config->cluster->num_replicas; ++i) {
			set_max(&state->gst_vector[i], gst_vector[i]);
		}
	}
	set_max(&state->dependency_vector[replica], dependency_time);
}

static void max_gst_and_dependency_vector(gr_tsp* max_vector, client_state_grv_t* state)
{
	for (unsigned int i = 0; i < state->config->cluster->num_replicas; ++i) {
		max_vector[i] = state->gst_vector[i] > state->dependency_vector[i]
			? state->gst_vector[i]
			: state->dependency_vector[i];
	}
}

static void grv_client_get_request(client_state_t *client, gr_key key)
{
	client_state_grv_t *state = client_protocol_state(client);
	lpid_t server_lpid = client_lpid_for_key(state->config, key);
	grv_get_request_t *request = grv_get_request_new(
			state->config->cluster->num_replicas);
	request->client_lpid = state->config->lpid;
	request->proxy_lpid = NO_PROXY_LPID;
	request->key = key;
	request->gst_vector_size = state->config->cluster->num_replicas;
	memcpy(grv_get_request_gst_vector(request), state->gst_vector,
			state->config->cluster->num_replicas * sizeof(*state->gst_vector));
	client_begin_request(client);
	client_send(client, server_lpid, GRV_GET_REQUEST, &request->message);
	free(request);
}

static void grv_client_put_request(client_state_t *client, gr_key key, gr_value value)
{
	client_state_grv_t *state = client_protocol_state(client);
	lpid_t server_lpid = client_lpid_for_key(state->config, key);
	grv_put_request_t *request = grv_put_request_new(
			state->config->cluster->num_replicas);
	request->client_lpid = state->config->lpid;
	request->proxy_lpid = NO_PROXY_LPID;
	request->key = key;
	request->value = value;
	max_gst_and_dependency_vector(grv_put_request_dependency_vector(request), state);
	client_begin_request(client);
	client_send(client, server_lpid, GRV_PUT_REQUEST, &request->message);
	free(request);
}

static void grv_client_rotx_request(client_state_t *client, gr_key *keys,
		unsigned int num_keys)
{
	client_state_grv_t *state = client_protocol_state(client);
	grv_rotx_request_t *request = grv_rotx_request_new(
			num_keys, state->config->cluster->num_replicas);
	request->client_lpid = state->config->lpid;
	request->dependency_time = state->dependency_vector[state->config->replica];
	memcpy(grv_rotx_request_keys(request), keys, num_keys * sizeof(gr_key));
	memcpy(grv_rotx_request_gst_vector(request), state->gst_vector,
			request->gst_vector_size * sizeof(*state->gst_vector));
	lpid_t server_lpid = lpid_of_any_partition(client);
	client_begin_request(client);
	client_send(client, server_lpid, GRV_ROTX_REQUEST, &request->message);
	free(request);
}

static void get_response(client_state_t *client, grv_get_response_t *response)
{
	client_state_grv_t *state = client_protocol_state(client);
	client_finish_request(client, get_request_type);
	assert(response->gst_vector_size == state->config->cluster->num_replicas);
	update_times(state, grv_get_response_gst_vector(response),
			response->update_timestamp, response->source_replica);
	state->config->funcs->on_get_response(
			client_workload_state_ptr(client), client, response->value);
}

static void put_response(client_state_t *client, grv_put_response_t *response)
{
	client_state_grv_t *state = client_protocol_state(client);
	client_finish_request(client, put_request_type);
	update_times(state, NULL, response->update_time, response->source_replica);
	state->config->funcs->on_put_response(
			client_workload_state_ptr(client), client);
}

static void rotx_response(client_state_t *client, grv_rotx_response_t *response)
{
	client_state_grv_t *state = client_protocol_state(client);
	client_finish_request(client, rotx_request_type);
	update_dependency_vector(state, grv_rotx_response_dependency_vector(response));
	state->config->funcs->on_rotx_response(
			client_workload_state_ptr(client), client,
			grv_rotx_response_values(response), response->num_values);
}

static int grv_client_protocol_process_event(client_state_t *client, simtime_t now,
		unsigned int event_type, void *data, size_t data_size)
{
	(void) now; // Unused parameters
	(void) data_size;

	switch (event_type) {
		case GRV_GET_RESPONSE:
			get_response(client, data);
			break;
		case GRV_PUT_RESPONSE:
			put_response(client, data);
			break;
		case GRV_ROTX_RESPONSE:
			rotx_response(client, data);
			break;
		default:
			return 0;
	}
	return 1;
}

client_functions_t grv_client_funcs = {
	grv_client_init,
	grv_client_get_request,
	grv_client_put_request,
	grv_client_rotx_request,
	grv_client_protocol_process_event
};
