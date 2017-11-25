#define  server_protocols_grv_grv_c
#include "server/protocols/gr/gr.h"
#include "event.h"
#include "messages/grv.h"
#include "parameters.h"
#include "queue.h"
#include "server/protocols/grv/getput.h"
#include "server/protocols/grv/gst.h"
#include "server/protocols/grv/heartbeat.h"
#include "server/protocols/grv/replication.h"
#include "server/protocols/grv/rotx.h"
#include "server/protocols/grv/slice.h"

static DEFINE_PROTOCOL_PARAMETER_FUNC(clock_interval, double, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_clock_tick_time, "gr");

static server_state_t *grv_allocate_state(void)
{
	return malloc(sizeof(grv_server_state_t));
}

static void grv_init_state(server_state_t *state_)
{
	grv_server_state_t *state = (grv_server_state_t*) state_;
	unsigned int num_replicas = state->config->cluster->num_replicas;
	state->version_vector = calloc(num_replicas, sizeof(gr_tsp));
	state->gst_vector = calloc(num_replicas, sizeof(*state->gst_vector));
	state->min_lst_vector = calloc(num_replicas, sizeof(*state->min_lst_vector));
	state->lst_received = calloc(state->config->tree_fanout, sizeof(int));
	state->num_snapshot_states = 0;
	state->snapshot_states = NULL;
	state->forwarded_get_id = server_stats_counter_new(&state->server_state,
			"forwarded get requests");
	state->forwarded_put_id = server_stats_counter_new(&state->server_state,
			"forwarded put requests");
	state->lock_gsv = cpu_lock_new(state->cpu);
	state->lock_vv = cpu_lock_new(state->cpu);
	ptr_array_init(&state->put_states);
	ptr_array_init(&state->rotx_states);

	state->replica_update_queues = malloc(num_replicas * sizeof(queue_t*));
	for (unsigned int i = 0; i < num_replicas; ++i) {
		state->replica_update_queues[i] = queue_new();
	}

	if (server_is_leaf_partition(&state->server_state)) {
		grv_schedule_gst_computation_start(state);
	}

	server_schedule_self(&state->server_state, clock_interval(),
			GRV_CLOCK_TICK, NULL, 0);
}

static void grv_process_clock_tick_locked(grv_server_state_t *state);
static void grv_process_clock_tick_unlocked(grv_server_state_t *state,
		int* send_time_ptr);

static void grv_process_clock_tick(grv_server_state_t *state)
{
	cpu_add_time(state->cpu, process_clock_tick_time());
	if (state->clock >=
			state->version_vector[state->config->replica] + clock_interval()) {
		cpu_lock_lock(state->cpu, state->lock_vv, GRV_CLOCK_TICK_LOCKED,
				NULL, 0);
	} else {
		int send_time = 0;
		grv_process_clock_tick_unlocked(state, &send_time);
	}
}

static void grv_process_clock_tick_locked(grv_server_state_t *state)
{
	state->version_vector[state->config->replica] = state->clock;
	int send_time = 1;
	cpu_lock_unlock(state->cpu, state->lock_vv, GRV_CLOCK_TICK_UNLOCKED,
			&send_time, sizeof(int));
}

static void grv_process_clock_tick_unlocked(grv_server_state_t *state,
		int* send_time_ptr)
{
	grv_send_heartbeat(state, *send_time_ptr);
	server_schedule_self(&state->server_state, clock_interval(),
			GRV_CLOCK_TICK, NULL, 0);
}

static int grv_process_event(server_state_t *state_, unsigned int event_type,
		void *data, size_t data_size)
{
	(void) data_size;
	grv_server_state_t *state = (grv_server_state_t*) state_;

	switch (event_type) {
		case GRV_GET_REQUEST:
			grv_process_get_request(state, data);
			break;
		case GRV_FORWARDED_GET_REQUEST_LOCKED:
			grv_process_forwarded_get_request_locked(state, data);
			break;
		case GRV_GET_REQUEST_LOCKED:
			grv_process_get_request_locked(state, data);
			break;
		case GRV_GET_REQUEST_UNLOCKED:
			grv_process_get_request_unlocked(state, data);
			break;
		case GRV_GET_RESPONSE:
			grv_process_get_response(state, data);
			break;
		case GRV_PUT_REQUEST:
			grv_process_put_request(state, data);
			break;
		case GRV_PUT_REQUEST_LOCKED:
			grv_process_put_request_locked(state, data);
			break;
		case GRV_PUT_REQUEST_UNLOCKED:
			grv_process_put_request_unlocked(state, data);
			break;
		case GRV_PUT_RESPONSE:
			grv_process_put_response(state, data);
			break;
		case GRV_REPLICA_UPDATE:
			grv_process_replica_update(state, data);
			break;
		case GRV_REPLICA_UPDATE_VV_LOCKED:
			grv_process_replica_update_vv_locked(state, data);
			break;
		case GRV_REPLICA_UPDATE_VV_UNLOCKED:
			grv_process_replica_update_vv_unlocked(state, data);
			break;
		case GRV_LST_FROM_LEAF:
			grv_process_lst_from_leaf(state, data);
			break;
		case GRV_LST_FROM_LEAF_ROOT_LOCKED:
			grv_process_lst_from_leaf_root_locked(state);
			break;
		case GRV_LST_FROM_LEAF_ROOT_UNLOCKED:
			grv_process_lst_from_leaf_root_unlocked(state);
			break;
		case GRV_GST_FROM_ROOT:
			grv_process_gst_from_root(state, data);
			break;
		case GRV_GST_FROM_ROOT_LOCKED:
			grv_process_gst_from_root_locked(state, data);
			break;
		case GRV_GST_FROM_ROOT_UNLOCKED:
			grv_process_gst_from_root_unlocked(state);
			break;
		case GRV_START_GST_COMPUTATION:
			grv_process_start_gst_computation(state);
			break;
		case GRV_CLOCK_TICK:
			grv_process_clock_tick(state);
			break;
		case GRV_CLOCK_TICK_LOCKED:
			grv_process_clock_tick_locked(state);
			break;
		case GRV_CLOCK_TICK_UNLOCKED:
			grv_process_clock_tick_unlocked(state, data);
			break;
		case GRV_HEARTBEAT:
			grv_process_heartbeat(state, data);
			break;
		case GRV_HEARTBEAT_LOCKED:
			grv_process_heartbeat_locked(state, data);
			break;
		case GRV_SLICE_REQUEST:
			grv_process_slice_request(state, data);
			break;
		case GRV_SLICE_REQUEST_LOCKED:
			grv_process_slice_request_locked(state, data);
			break;
		case GRV_SLICE_REQUEST_UNLOCKED:
			grv_process_slice_request_unlocked(state, data);
			break;
		case GRV_SLICE_RESPONSE:
			grv_process_slice_response(state, data);
			break;
		case GRV_ROTX_REQUEST:
			grv_process_rotx_request(state, data);
			break;
		case GRV_ROTX_REQUEST_LOCKED:
			grv_process_rotx_request_locked(state, data);
			break;
		case GRV_ROTX_REQUEST_UNLOCKED:
			grv_process_rotx_request_unlocked(state, data);
			break;
		default:
			return 0;
	}
	return 1;
}

server_functions_t grv_server_funcs = {
	grv_allocate_state,
	grv_init_state,
	grv_process_event,
};
