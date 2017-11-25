#include "server.h"
#include "common.h"
#include "cpu/cpu.h"
#include "cpu/stats.h"
#include "event.h"
#include "gentle_rain.h"
#include "messages/message.h"
#include "network.h"
#include "output.h"
#include "parameters.h"
#include "protocols.h"
#include "server/stats.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <errno.h>
#include <json.h>
#include <time.h>

static DEFINE_TIMING_FUNC(server_send_time);
static DEFINE_TIMING_FUNC(server_send_per_byte_time);

static void server_init(lpid_t lpid, simtime_t now, server_state_t *state)
{
	state->config = lp_config[lpid];
	state->clock = now;
	state->now = now;
	state->store = store_new();
	state->stats = server_stats_new();
	state->start_time = now;
	state->finished = 0;
	state->network = network_init(state->config->network);

	// Set network delay to all other servers
	struct json_object *network_obj = param_get_object_root("network");
	struct json_object *inter_replica_delay_matrix =
		param_get_double_matrix(network_obj, "inter_datacenter_delay",
				state->config->cluster->num_replicas,
				state->config->cluster->num_replicas);
	double intra_datacenter_network_delay = param_get_double(
			network_obj, "intra_datacenter_delay");
	double self_network_delay = param_get_double(network_obj, "self_delay");
	void set_network_delay(lpid_t other_lpid, replica_t replica, partition_t partition) {
		if (replica == state->config->replica) {
			network_set_delay(state->config->network, state->config->lpid,
					other_lpid, intra_datacenter_network_delay);
		} else if (partition == state->config->partition) {
			double delay = param_get_double_matrix_element(
						inter_replica_delay_matrix, state->config->replica,
						replica);
			network_set_delay(state->config->network, state->config->lpid,
					other_lpid, delay);
		}
	}
	foreach_server(state->config->cluster, set_network_delay);
	network_set_delay(state->config->network, lpid, lpid, self_network_delay);
	double transmission_rate = param_get_double(network_obj, "transmission_rate");
	network_set_transmission_rate(state->config->network, lpid, transmission_rate);

	// Setup CPU
	char output_prefix[PATH_MAX];
	output_server_file_path(state->config->replica, state->config->partition,
			"", output_prefix, PATH_MAX);
	state->cpu = cpu_setup(lpid, now, state, output_prefix, state->config->num_cores);

	// Initialize clock skew
	server_clock_skew_init(&state->clock_skew, state->config->cluster);

	server_funcs->init_state((void*) state);
}

static void server_process_event(lpid_t lpid, simtime_t now, unsigned int event_type,
		void *data, size_t data_size, void *state)
{
	(void) data_size; // Unused parameter
	server_state_t *server_state = (server_state_t*) state;
	if (server_state != NULL) {
		assert(lpid == server_state->config->lpid);
		assert(now >= server_state->now);
		server_state->now = now;
		server_state->clock = server_clock_skew(&server_state->clock_skew, now);
	}
	assert(event_type == INIT || server_state->config->lpid == lpid);

	switch (event_type) {
		case INIT:
			server_state = server_funcs->allocate_state();
			SetState(server_state);
			server_init(lpid, now, server_state);
			break;
		default:
			if (!server_funcs->process_event(state, event_type, data, data_size)) {
				fprintf(stderr,
						"lp %d (%s) received an unhandled event of type %s at %f\n",
						lpid, lp_name(lpid), event_name(event_type), now);
				exit(1);
			}
	}
}

static void output(server_state_t *state)
{
	// json-c doesn't support streaming, if this ends up being a problem,
	// consider porting to yajl

	/* Build the JSON document */
	struct json_object *doc = json_object_new_object();
	struct json_object *obj = json_object_new_object();
	json_object_object_add(doc, "stats", obj);
	server_stats_output(state, obj);
	obj = json_object_new_object();
	json_object_object_add(doc, "cpu", obj);
	cpu_stats_output(state->cpu, obj);
	obj = json_object_new_object();
	json_object_object_add(doc, "network", obj);
	network_stats_output(state->network, obj, state->now);

	/* Write the file */
	char path[PATH_MAX];
	if (output_server_file_path(state->config->replica, state->config->partition,
				".json", path, PATH_MAX) >= PATH_MAX) {
		fprintf(stderr, "Not writing statistics: filename is too long.");
	} else {
		if (json_object_to_file_ext(path, doc, JSON_C_TO_STRING_PRETTY)) {
			fprintf(stderr, "Couldn't write %s: %s\n",
					path, strerror(errno));
		}
	}
	json_object_put(doc);
}

static int server_on_gvt(lpid_t lpid, void *snapshot)
{
	(void) lpid; // Unused parameter
	server_state_t* state = snapshot;

	if (state->now > app_params.stop_after_simulated_seconds
			|| (app_params.stop_after_real_time && time(NULL) > app_params.stop_after_real_time)) {
		if (!state->finished) {
			state->finished = 1;
			output(state);
		}
		return 1;
	}
	return 0;
}

void server_schedule_self(server_state_t *state, simtime_t delay,
		unsigned int event_type, void *data, size_t data_size)
{
	assert(data_size <  UINT_MAX);
	assert(!cpu_lock_called(state->cpu));
	simtime_t time = state->now + cpu_elapsed_time(state->cpu) + delay;
	ScheduleNewEvent(state->config->lpid, time, event_type, data,
			(unsigned int) data_size);
}

void server_send(server_state_t *state, lpid_t to_lpid,
		unsigned int event_type, message_t *message)
{
	assert(!cpu_lock_called(state->cpu));
	cpu_add_time(state->cpu, server_send_time()
			+ (simtime_t) message->simulated_size * server_send_per_byte_time());
	simtime_t time = state->now + cpu_elapsed_time(state->cpu);
	network_send(state->network, state->config->lpid, to_lpid, time,
			event_type, message, message->size, message->simulated_size);
}

void server_setup(lpid_t lpid, cluster_config_t *cluster, replica_t replica,
		partition_t partition, network_config_t *network,
		unsigned int tree_fanout, unsigned int num_cores)
{
	assert(replica < cluster->num_replicas);
	assert(partition < cluster->num_partitions);
	void *__real_malloc(size_t size);

	server_config_t *config = __real_malloc(sizeof(server_config_t));
	config->cluster = cluster;
	config->lpid = lpid;
	config->replica = replica;
	config->partition = partition;
	config->network = network;
	config->tree_fanout = tree_fanout;
	config->num_cores = num_cores;
	lp_config[lpid] = config;

	register_callbacks(lpid, server_process_event, server_on_gvt);
}

unsigned int server_parent_partition_id(server_state_t *state)
{
	assert(state->config->partition != 0);
	return (state->config->partition - 1) / state->config->tree_fanout;
}

unsigned int server_child_partition_index(server_state_t *state,
		unsigned int partition_id)
{
	assert(partition_id != 0);
	return (partition_id - 1) % state->config->tree_fanout;
}

unsigned int server_first_child_id(server_state_t *state)
{
	return state->config->partition * state->config->tree_fanout + 1;
}

unsigned int server_last_child_id(server_state_t *state)
{
	return state->config->partition * state->config->tree_fanout
		+ state->config->tree_fanout;
}

int server_is_leaf_partition(server_state_t *state)
{
	unsigned int first_child_id = server_first_child_id(state);
	return (first_child_id >= state->config->cluster->num_partitions);
}

unsigned int server_num_children_partitions(server_state_t *state)
{
	unsigned int num_children = 0;
	unsigned int last_child_id = server_last_child_id(state);
	unsigned int first_child_id = server_first_child_id(state);
	if (last_child_id < state->config->cluster->num_partitions) {
		num_children =  state->config->tree_fanout;
	} else {
		if (first_child_id < state->config->cluster->num_partitions) {
			num_children = state->config->tree_fanout
				- (last_child_id - state->config->cluster->num_partitions + 1);
		}
	}
	return num_children;
}

void server_foreach_children_partition(server_state_t *state,
		void (*f)(partition_t partition, void *data), void *data)
{
	for (unsigned int i = server_first_child_id(state);
			i <= server_last_child_id(state)
				&& i < state->config->cluster->num_partitions;
			++i) {
		f(i, data);
	}
}
