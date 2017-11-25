#define client_workloads_get_put_rr_c
#include "client/workloads/get_put_rr.h"
#include "client/client.h"
#include "common.h"
#include "parameters.h"
#include <ROOT-Sim.h>

/* This client implementation performs a GET for a key on each partition and
 * then performs a PUT. The partitions are selected in a round-robin fashion */

struct state {
	cluster_config_t* cluster;
	partition_t last_get_partition;
	partition_t last_put_partition;
	unsigned int num_get_since_put;
	double target_request_time;
	int waiting;
};

#define get_state(data) \
	((struct state*) *data)

static void tick(struct state *state, client_state_t* cs)
{
	unsigned int num_partitions = state->cluster->num_partitions;
	if (state->num_get_since_put == num_partitions) {
		// Put request
		state->num_get_since_put = 0;
		unsigned int put_partition =
			inc_range(state->last_put_partition, 0, num_partitions - 1);
		gr_value value;
		randomize_value(&value);
		client_put_request(cs,
				client_random_key_on_partition(cs, put_partition), value);
		state->last_put_partition = put_partition;
	} else {
		// Get request
		unsigned int get_partition =
			inc_range(state->last_get_partition, 0, num_partitions - 1);
		client_get_request(cs,
				client_random_key_on_partition(cs, get_partition));
		state->last_get_partition = get_partition;
		++state->num_get_since_put;
	}
}

static void on_init(void **data, client_state_t* cs, cluster_config_t *cluster)
{
	struct state *state = malloc(sizeof(struct state));
	state->cluster = cluster;
	state->last_get_partition = random_uint(0, cluster->num_partitions);
	state->last_put_partition = random_uint(0, cluster->num_partitions);
	state->num_get_since_put = 0;
	state->target_request_time = 0;
	//state->target_request_time = 1.0 / 687.5;
	*data = state;
	tick(state, cs);
}

static simtime_t thinking_time(struct state *state, client_state_t *cs)
{
	simtime_t t = client_thinking_time();
	if (state->target_request_time > 0) {
		simtime_t request_duration = client_last_request_duration(cs);
		if (request_duration < state->target_request_time) {
			t = state->target_request_time - request_duration;
		} else {
			t = 0;
		}
		//fprintf(stderr, "target %g, last %g, t %g\n", state->target_request_time, request_duration, t);
	}
	return t;
}

static void on_get_response(void **data, client_state_t *cs, gr_value value)
{
	(void) value;
	simtime_t think_time = thinking_time(get_state(data), cs);
	client_schedule_tick(cs, think_time);
}

static void on_put_response(void **data, client_state_t *cs)
{
	(void) data;
	simtime_t think_time = thinking_time(get_state(data), cs);
	client_schedule_tick(cs, think_time);
}

static void on_tick(void **data, client_state_t *cs)
{
	tick(get_state(data), cs);
}

workload_functions_t workload_get_put_rr_funcs = {
	.on_init = on_init,
	.on_tick = on_tick,
	.on_get_response = on_get_response,
	.on_put_response = on_put_response,
	.on_rotx_response = NULL,
};
