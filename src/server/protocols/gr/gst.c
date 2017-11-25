#include "server/protocols/gr/gst.h"
#include "common.h"
#include "cpu/cpu.h"
#include "event.h"
#include "gentle_rain.h"
#include "parameters.h"
#include "server/protocols/gr/rotx.h"
#include "server/protocols/gr/stats.h"
#include "server/protocols/gr/store.h"
#include <assert.h>

static DEFINE_PROTOCOL_PARAMETER_FUNC(gst_interval, double, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(min_replica_version_per_replica_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_lst_from_leaf_end_per_replica_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_lst_from_leaf_per_replica_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(check_gst_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(update_gst_time, "gr");

static unsigned int count_lst_received(gr_server_state_t *state)
{
	unsigned int count = 0;
	for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
		if (state->lst_received[i]) ++count;
	}
	return count;
}

static gr_tsp min_replica_version(gr_server_state_t *state)
{
	gr_tsp min_version = state->version_vector[0];
	for (unsigned int i = 1; i < state->config->cluster->num_replicas; ++i) {
		gr_tsp version = state->version_vector[i];
		if (version < min_version) min_version = version;
	}
	cpu_add_time(state->cpu, state->config->cluster->num_replicas
			* min_replica_version_per_replica_time());
	return min_version;
}

void gr_process_lst_from_leaf(gr_server_state_t *state, gr_lst_from_leaf_t *leaf_lst)
{
	assert(leaf_lst->leaf_partition_id != 0);
	assert(!state->lst_received[server_child_partition_index(
				&state->server_state, leaf_lst->leaf_partition_id)]);
	unsigned int lst_received = count_lst_received(state);
	if (lst_received == 0 || state->min_lst > leaf_lst->lst) {
		state->min_lst = leaf_lst->lst;
	}
	cpu_add_time(state->cpu, process_lst_from_leaf_per_replica_time());
	state->lst_received[server_child_partition_index(
			&state->server_state, leaf_lst->leaf_partition_id)] = 1;
	if (lst_received == server_num_children_partitions(&state->server_state) - 1) {
		for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
			state->lst_received[i] = 0;
		}
		gr_tsp min_version = min_replica_version(state);
		if (min_version < state->min_lst) state->min_lst = min_version;
		cpu_add_time(state->cpu, process_lst_from_leaf_end_per_replica_time());

		if (state->config->partition == 0) {
			cpu_lock_lock(state->cpu, state->lock, GR_LST_FROM_LEAF_ROOT_LOCKED,
					NULL, 0);
		} else {
			gr_send_lst_to_parent(state);
		}
		// FIXME Currently garbage collection doesn't take cpu time
		// FIXME Garbage collection is currently disabled as it takes too much real time
		//garbage_collect(state);
	}
}

void gr_process_lst_from_leaf_root_locked(gr_server_state_t *state)
{
	gr_update_gst(state, state->min_lst);
	cpu_lock_unlock(state->cpu, state->lock, GR_LST_FROM_LEAF_ROOT_UNLOCKED,
			NULL, 0);
}

void gr_process_lst_from_leaf_root_unlocked(gr_server_state_t *state)
{
	gr_send_gst_to_children(state);
}

struct gr_send_gst_data {
	gr_server_state_t *state;
	gr_gst_from_root_t *root_gst;
};

static void gr_send_gst(partition_t partition, void *data_)
	{
		struct gr_send_gst_data * data = (struct gr_send_gst_data *) data_;
		gr_server_state_t *state = data->state;
		lpid_t child_lpid = cluster_get_lpid(state->config->cluster,
				state->config->replica, partition);
		server_send(&state->server_state, child_lpid, GR_GST_FROM_ROOT,
				&data->root_gst->message);
	}

void gr_send_gst_to_children(gr_server_state_t *state)
{
	gr_gst_from_root_t *root_gst = gr_gst_from_root_new();
	root_gst->gst = state->gst;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) root_gst->simulated_size);
	struct gr_send_gst_data data = {
		.state = state,
		.root_gst = root_gst,
	};
	server_foreach_children_partition(&state->server_state, gr_send_gst, &data);
	free(root_gst);
}

void gr_send_lst_to_parent(gr_server_state_t *state)
{
	assert(state->config->partition != 0);
	gr_lst_from_leaf_t *leaf_lst = gr_lst_from_leaf_new();
	leaf_lst->lst = state->min_lst;
	leaf_lst->leaf_partition_id = state->config->partition;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) leaf_lst->simulated_size);
	for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
		state->lst_received[i] = 0;
	}
	unsigned int parent_partition = server_parent_partition_id(&state->server_state);
	lpid_t parent_lpid = cluster_get_lpid(state->config->cluster,
			state->config->replica, parent_partition);
	server_send(&state->server_state, parent_lpid, GR_LST_FROM_LEAF,
			&leaf_lst->message);
	free(leaf_lst);
}

void gr_schedule_gst_computation_start(gr_server_state_t *state)
{
	server_schedule_self(&state->server_state, gst_interval(),
			GR_START_GST_COMPUTATION, NULL, 0);
}

void gr_process_gst_from_root(gr_server_state_t *state, gr_gst_from_root_t  *root_gst)
{
	cpu_lock_lock(state->cpu, state->lock, GR_GST_FROM_ROOT_LOCKED,
			root_gst, root_gst->size);
}

void gr_process_gst_from_root_locked(gr_server_state_t *state,
		gr_gst_from_root_t *root_gst)
{
	gr_update_gst(state, root_gst->gst);
	//FIXME See above FIXME near line 63
	//garbage_collect(state);
	cpu_lock_unlock(state->cpu, state->lock, GR_GST_FROM_ROOT_UNLOCKED,
			root_gst, root_gst->size);
}

void gr_process_gst_from_root_unlocked(gr_server_state_t *state,
		gr_gst_from_root_t *root_gst)
{
	if (server_is_leaf_partition(&state->server_state)) {
		cpu_allow_no_time(state->cpu);
		gr_schedule_gst_computation_start(state);
	} else {
		struct gr_send_gst_data data = {
			.state = state,
			.root_gst = root_gst,
		};
		server_foreach_children_partition(&state->server_state, gr_send_gst, &data);
	}
}

void gr_process_start_gst_computation(gr_server_state_t *state)
{
	assert(server_is_leaf_partition(&state->server_state));
	if (state->config->partition == 0) {
		// Only one partition
		gr_update_gst(state, state->version_vector[0]);
		gr_schedule_gst_computation_start(state);
		return;
	}
	gr_lst_from_leaf_t *leaf_lst = gr_lst_from_leaf_new();
	leaf_lst->lst = min_replica_version(state);
	leaf_lst->leaf_partition_id = state->config->partition;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) leaf_lst->simulated_size);
	lpid_t parent_lpid = cluster_get_lpid(
			state->config->cluster, state->config->replica,
			server_parent_partition_id(&state->server_state));
	server_send(&state->server_state, parent_lpid, GR_LST_FROM_LEAF,
			&leaf_lst->message);
	free(leaf_lst);
}

int gr_gst_need_update(gr_server_state_t *state, gr_tsp gst)
{
	cpu_add_time(state->cpu, check_gst_time());
	return state->gst < gst;
}

void gr_update_gst(gr_server_state_t *state, gr_tsp gst)
{
	cpu_add_time(state->cpu, update_gst_time());
	if (state->gst < gst) {
		gr_stats_gst_update(state, state->gst, gst);
		state->gst = gst;
		gr_rotx_on_gst_updated(state);
	}
}
