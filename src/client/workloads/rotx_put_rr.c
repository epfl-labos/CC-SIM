#define client_workloads_rotx_put_rr_c
#include "client/workloads/rotx_put_rr.h"
#include "client/client.h"
#include "common.h"
#include "parameters.h"
#include <assert.h>

/* This workloads perfoms the following in a loop:
 *  - Perform num_rotx rotx containing keys_per_rotx key (if possible each key
 *    is chosen on a different partition)
 *  - Perform num_put put request on different partitions, choosing the most
 *    recently used in a rotx request partitions first.
 *
 *  The partition are chosen in a round-robbin fashion.
 */

static unsigned int keys_per_rotx;
static unsigned int num_rotx;
static unsigned int num_put;

enum state_enum {
	SLEEPING,
	WAITING_PUT_RESPONSE,
	WAITING_ROTX_RESPONSE,
};

struct state {
	cluster_config_t *cluster;
	enum state_enum state;
	partition_t rotx_partition;
	partition_t put_partition;
	unsigned int rotx_count;
	unsigned int put_count;
};

#define get_state(data) \
	((struct state *) *data)

static void do_rotx(struct state *state, client_state_t *cs)
{
	gr_key keys[keys_per_rotx];
	for (unsigned int i = 0; i < keys_per_rotx; ++i) {
		keys[i] = client_random_key_on_partition(cs, state->rotx_partition);
		state->rotx_partition = inc_range(state->rotx_partition,
				0, state->cluster->num_partitions - 1);
	}
	client_rotx_request(cs, keys, keys_per_rotx);
}

static void do_put(struct state *state, client_state_t *cs)
{
	gr_key key = client_random_key_on_partition(cs, state->put_partition);
	gr_value value;
	randomize_value(&value);
	client_put_request(cs, key, value);
	state->put_partition = state->put_partition == 0
		? state->cluster->num_partitions - 1
		: state->put_partition - 1;
}

static void on_tick(void **data, client_state_t *cs)
{
	struct state *state = get_state(data);
	assert(state->state == SLEEPING);
	assert(state->rotx_partition < state->cluster->num_partitions);
	if (state->rotx_count < num_rotx) {
		do_rotx(state, cs);
		++state->rotx_count;
		state->state = WAITING_ROTX_RESPONSE;
	} else {
		do_put(state, cs);
		++state->put_count;
		state->state = WAITING_PUT_RESPONSE;
		if (state->put_count < num_put) {
			state->put_count = 0;
			state->rotx_count = 0;
		}
	}
	assert(state->rotx_partition < state->cluster->num_partitions);
}

static void on_init(void **data, client_state_t *cs, cluster_config_t *cluster)
{
	struct state *state = malloc(sizeof(struct state));
	*data = state;
	state->state = SLEEPING;
	state->cluster = cluster;
	state->rotx_partition = random_uint(0, cluster->num_partitions - 1);
	state->put_partition = 0;
	state->rotx_count = 0;
	state->put_count = 0;
	if (keys_per_rotx == 0) {
		struct json_object *jobj = param_get_workload_object("rotx_put_rr");
		keys_per_rotx = param_get_uint(jobj, "values per rotx");
		num_rotx = param_get_uint(jobj, "number of rotx");
		num_put = param_get_uint(jobj, "number of put");
	}
	assert(keys_per_rotx > 0);
	on_tick(data, cs);
}

static void on_put_response(void **data, client_state_t *cs)
{
	struct state *state = get_state(data);
	assert(state->state == WAITING_PUT_RESPONSE);
	state->state = SLEEPING;
	client_schedule_tick(cs, client_thinking_time());
}

static void on_rotx_response(void **data, client_state_t *cs,
		gr_value *values, unsigned int num_values)
{
	(void) values, (void) num_values; // Unused parameters
	struct state * state = get_state(data);
	assert(state->state == WAITING_ROTX_RESPONSE);
	state->state = SLEEPING;
	client_schedule_tick(cs, client_thinking_time());
}

workload_functions_t workload_rotx_put_rr_funcs = {
	.on_init = on_init,
	.on_tick = on_tick,
	.on_put_response = on_put_response,
	.on_rotx_response = on_rotx_response,
};
