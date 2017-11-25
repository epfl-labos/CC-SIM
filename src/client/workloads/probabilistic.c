#define client_workloads_probabilistic_c
#include "client/workloads/probabilistic.h"
#include "client/client.h"
#include "common.h"
#include "parameters.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <inttypes.h>
#include <json.h>
#include <math.h>

static double get_threshold, put_threshold, rotx_threshold;
static unsigned int keys_per_rotx;

enum state_enum {
	SLEEPING,
	WAITING_RESPONSE,
	RESPONSE_RECEIVED,
};

struct state {
	enum state_enum state;
};

#define get_state(data) \
	((struct state*) *data)

static void on_tick(void **data, client_state_t *cs)
{
	struct state *state = get_state(data);
	if (state->state == SLEEPING) {
		state->state = WAITING_RESPONSE;
		gr_key key = client_random_key(cs);
		double r = Random();
		if (r < get_threshold) {
			client_get_request(cs, key);
		} else if (r < put_threshold) {
			gr_value value;
			randomize_value(&value);
			client_put_request(cs, key, value);
		} else {
			unsigned int num_keys = keys_per_rotx;
			gr_key *keys = client_random_keys(cs, num_keys);
			client_rotx_request(cs, keys, num_keys);
			free(keys);
		}
	} else if (state->state == RESPONSE_RECEIVED) {
		state->state = SLEEPING;
		client_schedule_tick(cs, client_thinking_time());
	} else {
		fprintf(stderr, "client_tick unexpectedly called while in state %d\n",
				(int) state->state);
		exit(1);
	}
}

static void on_init(void **data, client_state_t *cs, cluster_config_t *cluster)
{
	(void) cluster; // Unused parameter
	struct state *state = malloc(sizeof(struct state));
	state->state = SLEEPING;
	*data = state;

	if (get_threshold == 0.0) {
		struct json_object *jobj;
		jobj = param_get_workload_object("probabilistic");
		get_threshold = param_get_double(jobj, "get probability");
		put_threshold = param_get_double(jobj, "put probability") + get_threshold;
		rotx_threshold = param_get_double(jobj, "rotx probability") + put_threshold;
		if (fabs(rotx_threshold - 1) > 1e-6) {
			fprintf(stderr, "The probabilities of the workload don't sum to 1.\n");
			exit(1);
		}
		int n = param_get_int(jobj, "keys per rotx");
		assert(n > 0);
		keys_per_rotx = (unsigned int) n;
	}

	on_tick(data, cs);
}

static void on_get_response(void **data, client_state_t *cs, gr_value value)
{
	(void) value;
	struct state *state = get_state(data);
	state->state = SLEEPING;
	client_schedule_tick(cs, client_thinking_time());
}

static void on_put_response(void **data, client_state_t *cs)
{
	struct state *state = get_state(data);
	state->state = SLEEPING;
	client_schedule_tick(cs, client_thinking_time());
}

static void on_rotx_response(void **data, client_state_t *cs,
		gr_value *values, unsigned int num_values)
{
	(void) values;
	(void) num_values;
	struct state *state = get_state(data);
	state->state = SLEEPING;
	client_schedule_tick(cs, client_thinking_time());
}

workload_functions_t workload_probabilistic_funcs = {
	.on_init = on_init,
	.on_tick = on_tick,
	.on_get_response = on_get_response,
	.on_put_response = on_put_response,
	.on_rotx_response = on_rotx_response,
};
