#define server_protocols_gr_gr_c
#include "server/protocols/gr/gr.h"
#include "event.h"
#include "parameters.h"
#include "protocols.h"
#include "ptr_array.h"
#include "server/protocols/gr/getput.h"
#include "server/protocols/gr/gst.h"
#include "server/protocols/gr/heartbeat.h"
#include "server/protocols/gr/replication.h"
#include "server/protocols/gr/rotx.h"
#include "server/protocols/gr/slice.h"
#include "server/protocols/gr/snapshot.h"
#include "server/stats.h"

static DEFINE_PROTOCOL_PARAMETER_FUNC(clock_interval, double, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_clock_tick_time, "gr");

static server_state_t *gr_allocate_state(void)
{
	return malloc(sizeof(gr_server_state_t));
}

static void gr_init_state(server_state_t *state_)
{
	gr_server_state_t *state = (gr_server_state_t*) state_;
	unsigned int num_replicas = state->config->cluster->num_replicas;
	state->version_vector = calloc(num_replicas, sizeof(gr_tsp));
	state->gst = 0;
	state->min_lst = 0;
	state->lst_received = calloc(state->config->tree_fanout, sizeof(int));
	state->forwarded_get_id = server_stats_counter_new(&state->server_state,
			"forwarded get requests");
	state->forwarded_put_id = server_stats_counter_new(&state->server_state,
			"forwarded put requests");
	state->lock = cpu_lock_new(state->cpu);
	ptr_array_init(&state->get_states);
	ptr_array_init(&state->put_states);
	ptr_array_init(&state->snapshot_states);
	ptr_array_init(&state->rotx_states);

	state->replica_locks = malloc(num_replicas * sizeof(cpu_lock_id_t));
	for (unsigned int i = 0; i < num_replicas; ++i) {
		state->replica_locks[i] = cpu_lock_new(state->cpu);
	}

	state->replica_update_queues = malloc(num_replicas * sizeof(queue_t*));
	for (unsigned int i = 0; i < num_replicas; ++i) {
		state->replica_update_queues[i] = queue_new();
	}

	if (server_is_leaf_partition(&state->server_state)) {
		gr_schedule_gst_computation_start(state);
	}

	server_schedule_self(&state->server_state, clock_interval(),
			GR_CLOCK_TICK, NULL, 0);
}

static void gr_process_clock_tick(gr_server_state_t *state);
static void gr_process_clock_tick_locked(gr_server_state_t *state);
static void gr_process_clock_tick_unlocked(gr_server_state_t *state,
		int *send_time_ptr);

static void gr_process_clock_tick(gr_server_state_t *state)
{
	cpu_add_time(state->cpu, process_clock_tick_time());
	if (state->clock >= state->version_vector[state->config->replica] + clock_interval()) {
		cpu_lock_lock(state->cpu, state->lock, GR_CLOCK_TICK_LOCKED, NULL, 0);
	} else {
		int send_time = 0;
		gr_process_clock_tick_unlocked(state, &send_time);
	}
}

static void gr_process_clock_tick_locked(gr_server_state_t *state)
{
	cpu_add_time(state->cpu, process_clock_tick_time());
	int send_time = 0;
	if (state->clock >= state->version_vector[state->config->replica] + clock_interval()) {
		state->version_vector[state->config->replica] = state->clock;
		send_time = 1;
	}
	cpu_lock_unlock(state->cpu, state->lock, GR_CLOCK_TICK_UNLOCKED,
			&send_time, sizeof(int));
}

static void gr_process_clock_tick_unlocked(gr_server_state_t *state,
		int *send_time_ptr)
{
	gr_send_heartbeat(state, *send_time_ptr);
	server_schedule_self(&state->server_state, clock_interval(),
			GR_CLOCK_TICK, NULL, 0);
}

static int gr_process_event(server_state_t *state_, unsigned int event_type,
		void *data, size_t data_size)
{
	(void) data_size; // Unused parameter

	gr_server_state_t *state = (gr_server_state_t*) state_;
	switch (event_type) {
		case GR_GET_REQUEST:
			gr_process_get_request(state, data);
			break;
		case GR_FORWARD_GET_REQUEST_LOCKED:
			gr_process_forward_get_request_locked(state, data);
			break;
		case GR_GET_REQUEST_LOCKED:
			gr_process_get_request_locked(state, data);
			break;
		case GR_GET_REQUEST_UNLOCKED:
			gr_process_get_request_unlocked(state, data);
			break;
		case GR_GET_RESPONSE:
			gr_process_get_response(state, data);
			break;
		case GR_PUT_REQUEST:
			gr_process_put_request(state, data);
			break;
		case GR_PUT_REQUEST_LOCKED:
			gr_process_put_request_locked(state, data);
			break;
		case GR_PUT_REQUEST_UNLOCKED:
			gr_process_put_request_unlocked(state, data);
			break;
		case GR_PUT_RESPONSE:
			gr_process_put_response(state, data);
			break;
		case GR_REPLICA_UPDATE:
			gr_process_replica_update(state, data);
			break;
		case GR_REPLICA_UPDATE_LOCKED:
			gr_process_replica_update_locked(state, data);
			break;
		case GR_REPLICA_UPDATE_UNLOCKED:
			gr_process_replica_update_unlocked(state, data);
			break;
		case GR_LST_FROM_LEAF:
			gr_process_lst_from_leaf(state, data);
			break;
		case GR_LST_FROM_LEAF_ROOT_LOCKED:
			gr_process_lst_from_leaf_root_locked(state);
			break;
		case GR_LST_FROM_LEAF_ROOT_UNLOCKED:
			gr_process_lst_from_leaf_root_unlocked(state);
			break;
		case GR_GST_FROM_ROOT:
			gr_process_gst_from_root(state, data);
			break;
		case GR_GST_FROM_ROOT_LOCKED:
			gr_process_gst_from_root_locked(state, data);
			break;
		case GR_GST_FROM_ROOT_UNLOCKED:
			gr_process_gst_from_root_unlocked(state, data);
			break;
		case GR_START_GST_COMPUTATION:
			gr_process_start_gst_computation(state);
			break;
		case GR_CLOCK_TICK:
			gr_process_clock_tick(state);
			break;
		case GR_CLOCK_TICK_LOCKED:
			gr_process_clock_tick_locked(state);
			break;
		case GR_CLOCK_TICK_UNLOCKED:
			gr_process_clock_tick_unlocked(state, data);
			break;
		case GR_HEARTBEAT:
			gr_process_heartbeat(state, data);
			break;
		case GR_HEARTBEAT_LOCKED:
			gr_process_heartbeat_locked(state, data);
			break;
		case GR_GET_SNAPSHOT_REQUEST:
			gr_process_get_snapshot_request(state, data);
			break;
		case GR_GET_SNAPSHOT_REQUEST_LOCKED:
			gr_process_get_snapshot_request_locked(state, data);
			break;
		case GR_GET_SNAPSHOT_REQUEST_UNLOCKED:
			gr_process_get_snapshot_request_unlocked(state, data);
			break;
		case GR_SLICE_REQUEST:
			gr_process_slice_request(state, data);
			break;
		case GR_SLICE_REQUEST_LOCKED:
			gr_process_slice_request_locked(state, data);
			break;
		case GR_SLICE_REQUEST_UNLOCKED:
			gr_process_slice_request_unlocked(state, data);
			break;
		case GR_SLICE_RESPONSE:
			gr_process_slice_response(state, data);
			break;
		case GR_SLICE_RESPONSE_LOCKED:
			gr_process_slice_response_locked(state, data);
			break;
		case GR_SLICE_RESPONSE_UNLOCKED:
			gr_process_slice_response_unlocked(state, data);
			break;
		case GR_ROTX_REQUEST:
			gr_process_rotx_request(state, data);
			break;
		default:
			return 0;
	}
	return 1;
}

server_functions_t gr_server_funcs = {
	gr_allocate_state,
	gr_init_state,
	gr_process_event,
};
