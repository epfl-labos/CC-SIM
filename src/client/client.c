#include "client.h"
#include "client/key_distribution.h"
#include "client/workloads/get_put_rr.h"
#include "client/workloads/probabilistic.h"
#include "common.h"
#include "event.h"
#include "gentle_rain.h"
#include "network.h"
#include "output.h"
#include "parameters.h"
#include "protocols.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <errno.h>
#include <json.h>
#include <limits.h>
#include <math.h>
#include <time.h>

typedef struct {
	int count;
	double latency_sum;
} request_stats_t;

struct client_state {
	int state;
	void *state_data;
	void *protocol_state;
	unsigned int last_used_partition;
	const client_config_t *config;
	network_state_t *network;
	simtime_t start_time;
	simtime_t now;
	simtime_t request_time;
	simtime_t last_request_duration;
	int finished;
	unsigned int num_request_types;
	char **request_type_names;
	request_stats_t **request_stats;
};

/* Register a new request type to be used with client_finish_request(). */
int client_register_request_type(client_state_t *state, const char *name)
{
	assert(state->num_request_types <= INT_MAX);
	int id = (int) state->num_request_types;
	++state->num_request_types;
	state->request_type_names = realloc(state->request_type_names,
			sizeof(*state->request_type_names) * state->num_request_types);
	state->request_type_names[id] = malloc(strlen(name) + 1);
	strcpy(state->request_type_names[id], name);
	state->request_stats = realloc(state->request_stats,
			sizeof(*state->request_stats) * state->num_request_types);
	request_stats_t *stats = malloc(sizeof(request_stats_t));
	stats->count = 0;
	stats->latency_sum = 0;
	state->request_stats[id] = stats;
	return id;
}

/* To be called by the protocol implementation when doing a request. */
void client_begin_request(client_state_t *state)
{
	assert(state->request_time == -1);
	state->request_time = state->now;
}

/* To be called by the protocol implementation when receiving the response to a
 * previous request. Computes statistics for the current request. Statistics are
 * computed separately for each request type. Use client_register_request_type()
 * to define a new request_type. */
simtime_t client_finish_request(client_state_t *state, int request_type)
{
	// If the following assertion is false, this typically means either the
	// protocol implementation didn't call client_begin_request() when it sent
	// the request or the client unexpectedly got a second response from a
	// server.
	assert(state->request_time != -1);
	// The request type must be registered using client_register_request_type()
	// first.
	assert((unsigned int) request_type < state->num_request_types);
	state->last_request_duration = state->now - state->request_time;
	state->request_time = -1;
	if (state->now >= app_params.ignore_initial_seconds) {
		request_stats_t *request_stats = state->request_stats[request_type];
		++request_stats->count;
		request_stats->latency_sum += state->last_request_duration;
	}
	return state->last_request_duration;
}

void **client_workload_state_ptr(client_state_t *state)
{
	return &state->state_data;
}

void *client_protocol_state(client_state_t *state)
{
	return state->protocol_state;
}

simtime_t client_last_request_duration(client_state_t *state)
{
	return state->last_request_duration;
}

lpid_t lpid_of_any_partition(client_state_t *state)
{
	partition_t partition;
	if (state->config->tied_to_partition) {
		partition = state->config->partition;
	} else {
		state->last_used_partition = inc_range(state->last_used_partition, 0,
				state->config->cluster->num_partitions);
		partition = state->last_used_partition;
	}
	return cluster_get_lpid(state->config->cluster,
			state->config->replica, partition);
}

void client_schedule_tick(client_state_t *state, simtime_t delay)
{
	ScheduleNewEvent(state->config->lpid, state->now + delay,
			CLIENT_TICK, NULL, 0);
}

static void client_tick(client_state_t *state)
{
	state->config->funcs->on_tick(&state->state_data, state);
}

void client_send(client_state_t *state, lpid_t to_lpid, unsigned int
		event_type, message_t *message)
{
	network_send(state->network, state->config->lpid, to_lpid, state->now,
			event_type, message, message->size, message->simulated_size);
}

gr_key client_random_key(client_state_t *state)
{
	return state->config->random_key(state->config);
}

gr_key *client_random_keys(client_state_t *state, size_t num_keys)
{
	gr_key *keys = malloc(num_keys * sizeof(gr_key));
	for (size_t i = 0; i < num_keys; ++i) {
		keys[i] = client_random_key(state);
	}
	return keys;
}

gr_key client_random_key_on_partition(client_state_t *state, partition_t partition)
{
	return state->config->random_key_on_partition(state->config, partition);
}

static void client_init(lpid_t lpid, simtime_t now, client_state_t *state)
{
	state->config = lp_config[lpid];
	assert(state->config->cluster->num_partitions < (unsigned int) INT_MAX);
	state->last_used_partition = random_uint(0,
			state->config->cluster->num_partitions);
	state->network = network_init(state->config->network);
	state->start_time = now;
	state->now = now;
	state->request_time = -1;
	state->last_request_duration = 0;
	state->finished = 0;
	state->num_request_types = 0;
	state->request_type_names = NULL;
	state->request_stats = NULL;

	struct json_object *network_obj = param_get_object_root("network");
	double intra_datacenter_network_delay = param_get_double(
			network_obj, "intra_datacenter_delay");

	if (state->config->tied_to_partition) {
		lpid_t server_lpid = cluster_get_lpid(state->config->cluster,
				state->config->replica, state->config->partition);
		network_set_delay(state->config->network, lpid, server_lpid,
				intra_datacenter_network_delay);
		network_set_delay(state->config->network, server_lpid, lpid,
				intra_datacenter_network_delay);
	} else {
		// Set network delay with servers in the replica
		void set_network_delay(lpid_t server_lpid, partition_t partition) {
			(void) partition; // Unused parameter;
			network_set_delay(state->config->network, lpid, server_lpid,
					intra_datacenter_network_delay);
			network_set_delay(state->config->network, server_lpid, lpid,
					intra_datacenter_network_delay);
		}
		foreach_partition(state->config->cluster, state->config->replica,
				set_network_delay);
	}

	double transmission_rate = param_get_double(network_obj, "transmission_rate");
	network_set_transmission_rate(state->config->network, lpid, transmission_rate);

	state->protocol_state = client_funcs->init(state, state->config);
	state->config->funcs->on_init(
			&state->state_data, state, state->config->cluster);
}

static void client_process_event(lpid_t lpid, simtime_t now,
		unsigned int event_type, void *data, size_t data_size, void *state)
{
	(void) data_size; // Unused parameter
	client_state_t *client_state = (client_state_t*) state;
	if (client_state != NULL) {
		assert(lpid == client_state->config->lpid);
		assert(now >= client_state->now);
		client_state->now = now;
	}

	switch (event_type) {
		case INIT:
			if (now == 0) {
				/* Don't initialize every clients at the same time. */
				assert(data_size <  UINT_MAX);
				ScheduleNewEvent(lpid, Random() * 0.002 + now, event_type,
						data, (unsigned int) data_size);
			} else {
				client_state = malloc(sizeof(client_state_t));
				client_init(lpid, now, client_state);
				SetState(client_state);
			}
			break;
		case CLIENT_TICK:
			client_tick(client_state);
			break;
		default:
			if (!client_funcs->process_event(
						client_state, now, event_type, data, data_size))
			{
				fprintf(stderr,
						"lp %d (%s) received an unhandled event of type %s at %f\n",
						lpid, lp_name(lpid), event_name(event_type), now);
				exit(1);
			}
	}
}

#define div_or_zero(a, b) \
	(b ? a / b : 0)

static void client_stats_output(client_state_t *state)
{
	assert_gvt();
	simtime_t elapsed = state->now -
		fmax(state->start_time, app_params.ignore_initial_seconds);
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "name",
			json_object_new_string(lp_name(state->config->lpid)));
	for (unsigned int i = 0; i < state->num_request_types; ++i) {
		request_stats_t *s = state->request_stats[i];
		struct json_object *req_obj = json_object_new_object();
		json_object_object_add(req_obj, "count",
				json_object_new_int64(s->count));
		json_object_object_add(req_obj, "rate",
				json_object_new_double(s->count / elapsed));
		json_object_object_add(req_obj, "average latency",
				json_object_new_double(div_or_zero(s->latency_sum, s->count)));
		json_object_object_add(obj, state->request_type_names[i], req_obj);
	}

	char path[PATH_MAX];
	if (output_client_file_path(state->config->lpid, state->config->replica,
				"stats.json", path, PATH_MAX) >= PATH_MAX) {
		fprintf(stderr, "Not writing client statistics: filename is too long.");
	} else {
		if (json_object_to_file_ext(path, obj, JSON_C_TO_STRING_PRETTY)) {
			fprintf(stderr, "Couldn't write %s: %s\n",
					path, strerror(errno));
		}
	}
	json_object_put(obj);
}

static int client_on_gvt(lpid_t lpid, void *snapshot)
{
	(void) lpid; // Unused parameter
	client_state_t *state = snapshot;
	if (state->now > app_params.stop_after_simulated_seconds
			|| (app_params.stop_after_real_time && time(NULL) > app_params.stop_after_real_time)) {
		if (!state->finished) {
			state->finished = 1;
			client_stats_output(snapshot);
		}
		return 1;
	}
	return 0;
}

void client_setup(lpid_t lpid, cluster_config_t *cluster, replica_t replica,
		partition_t partition, network_config_t *network, const char* workload,
		const char *key_distribution, double zipf_skew)
{
	void *__real_malloc(size_t size);

	client_config_t *config = __real_malloc(sizeof(client_config_t));
	config->lpid = lpid;
	config->cluster = cluster;
	config->replica = replica;
	config->partition = partition;
	config->tied_to_partition = 1;
	config->network = network;
	config->zipf_skew = zipf_skew;

	// Choose the workload-specific functions according to the configuration
	for (int i = 0; ; ++i) {
		if (workloads[i].name == NULL) {
			fprintf(stderr, "Workload \"%s\" is unknown.\n", workload);
			exit(1);
		}
		if (!strcmp(workload, workloads[i].name)) {
			config->funcs = workloads[i].functions;
			break;
		}
	}

	for (int i = 0; ; ++i) {
		if (client_key_distributions[i].name == NULL) {
			fprintf(stderr, "Key distribution \"%s\" is unknown.\n", key_distribution);
			exit(1);
		}
		if (!strcmp(key_distribution, client_key_distributions[i].name)) {
			config->random_key =
				client_key_distributions[i].random_key;
			config->random_key_on_partition =
				client_key_distributions[i].random_key_on_partition;
			break;
		}
	}

	lp_config[lpid] = config;
	register_callbacks(lpid, client_process_event, client_on_gvt);
}

DEFINE_TIMING_FUNC(client_thinking_time);
