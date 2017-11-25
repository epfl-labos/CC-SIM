#define client_protocols_gr_c
#include "client/protocols/gr.h"
#include "client/client.h"
#include "client/protocols/gr_common.h"
#include "common.h"
#include "event.h"

typedef struct {
	const client_config_t *config;
	gr_tsp dependency_time;  // Maximum update timestamp of all items accessed so far
	gr_gst gst;  // GST that the client is aware of
} client_state_gr_t;

int get_request_type, put_request_type, rotx_request_type;

static void *gr_client_protocol_init(client_state_t *client,
		const client_config_t *config)
{
	get_request_type = client_register_request_type(client, "get");
	put_request_type = client_register_request_type(client, "put");
	rotx_request_type = client_register_request_type(client, "rotx");
	client_state_gr_t *state = calloc(1, sizeof(client_state_gr_t));
	state->config = config;
	return state;
}

static void update_times(client_state_gr_t *state, gr_tsp gst, gr_tsp dependency_time)
{
	set_max(&state->dependency_time, dependency_time);
	set_max(&state->gst, gst);
}

static void gr_client_get_request(client_state_t *client, gr_key key)
{
	client_state_gr_t *state = client_protocol_state(client);
	lpid_t server_lpid = client_lpid_for_key(state->config, key);
	gr_get_request_t *request = gr_get_request_new();
	request->client_lpid = state->config->lpid;
	request->proxy_lpid = NO_PROXY_LPID;
	request->key = key;
	request->gst = state->gst;
	client_begin_request(client);
	client_send(client, server_lpid, GR_GET_REQUEST, &request->message);
	free(request);
}

static void gr_client_put_request(client_state_t *client, gr_key key, gr_value value)
{
	client_state_gr_t *state = client_protocol_state(client);
	lpid_t server_lpid = client_lpid_for_key(state->config, key);
	gr_tsp dependency_time = state->dependency_time;
	gr_put_request_t *request = gr_put_request_new();
	request->client_lpid = state->config->lpid;
	request->proxy_lpid = NO_PROXY_LPID;
	request->key = key;
	request->value = value;
	request->dependency_time = dependency_time;
	client_begin_request(client);
	client_send(client, server_lpid, GR_PUT_REQUEST, &request->message);
	free(request);
}

static void gr_client_rotx_request(client_state_t *client, gr_key *keys,
		unsigned int num_keys)
{
	client_state_gr_t *state = client_protocol_state(client);
	gr_get_rotx_request_t *request = gr_get_rotx_request_new(num_keys);
	request->client_lpid = state->config->lpid;
	memcpy(gr_get_rotx_request_keys(request), keys, num_keys * sizeof(gr_key));
	request->gst = state->gst;
	request->dependency_time = state->dependency_time;
	lpid_t server_lpid = lpid_of_any_partition(client);
	client_begin_request(client);
	client_send(client, server_lpid, GR_ROTX_REQUEST, &request->message);
	free(request);
}

static void get_response(client_state_t *client, gr_get_response_t *response)
{
	client_state_gr_t *state = client_protocol_state(client);
	client_finish_request(client, get_request_type);
	update_times(state, response->gst, response->update_timestamp);
	state->config->funcs->on_get_response(
			client_workload_state_ptr(client), client, response->value);
}

static void put_response(client_state_t *client, gr_put_response_t *response)
{
	client_state_gr_t *state = client_protocol_state(client);
	client_finish_request(client, put_request_type);
	update_times(state, 0, response->update_timestamp);
	state->config->funcs->on_put_response(
			client_workload_state_ptr(client), client);
}

static void rotx_response(client_state_t *client, gr_get_rotx_response_t *response)
{
	client_state_gr_t *state = client_protocol_state(client);
	client_finish_request(client, rotx_request_type);
	update_times(state, response->gst, response->update_timestamp);
	state->config->funcs->on_rotx_response(
			client_workload_state_ptr(client), client,
			gr_get_rotx_response_values(response), response->num_values);
}

static int gr_client_protocol_process_event(client_state_t *client, simtime_t now,
		unsigned int event_type, void *data, size_t data_size)
{
	(void) now; // Unused parameters
	(void) data_size;

	switch (event_type) {
		case GR_GET_RESPONSE:
			get_response(client, data);
			break;
		case GR_PUT_RESPONSE:
			put_response(client, data);
			break;
		case GR_ROTX_RESPONSE:
			rotx_response(client, data);
			break;
		default:
			return 0;
	}
	return 1;
}

client_functions_t gr_client_funcs = {
	gr_client_protocol_init,
	gr_client_get_request,
	gr_client_put_request,
	gr_client_rotx_request,
	gr_client_protocol_process_event
};
