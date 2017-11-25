#include "server/protocols/grv/gst.h"
#include "common.h"
#include "cpu/cpu.h"
#include "event.h"
#include "gentle_rain.h"
#include "messages/grv.h"
#include "parameters.h"
#include "server/protocols/grv/rotx.h"
#include "server/protocols/grv/stats.h"
#include "server/protocols/grv/store.h"
#include <assert.h>

static DEFINE_PROTOCOL_PARAMETER_FUNC(gst_interval, double, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(check_gst_vector_per_replica_time, "grv");
static DEFINE_PROTOCOL_TIMING_FUNC(process_lst_from_leaf_end_per_replica_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(process_lst_from_leaf_per_replica_time, "gr");
static DEFINE_PROTOCOL_TIMING_FUNC(update_gst_vector_per_replica_time, "grv");
static DEFINE_PROTOCOL_TIMING_FUNC(update_gst_vector_per_update_time, "grv");

static unsigned int count_lst_received(grv_server_state_t *state)
{
	unsigned int count = 0;
	for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
		if (state->lst_received[i]) ++count;
	}
	return count;
}

void grv_process_lst_from_leaf(grv_server_state_t *state,
		grv_lst_from_leaf_t *leaf_lst)
{
	assert(leaf_lst->leaf_partition_id != 0);
	assert(!state->lst_received[server_child_partition_index(
				&state->server_state, leaf_lst->leaf_partition_id)]);
	unsigned int lst_received = count_lst_received(state);
	unsigned int num_replicas = state->config->cluster->num_replicas;
	if (lst_received == 0) {
		memcpy(state->min_lst_vector, grv_lst_from_leaf_lst_vector(leaf_lst),
				num_replicas * sizeof(*state->min_lst_vector));
	} else {
		for (unsigned int i = 0; i < num_replicas; ++i) {
			set_min(&state->min_lst_vector[i],
					grv_lst_from_leaf_lst_vector(leaf_lst)[i]);
		}
	}
	cpu_add_time(state->cpu,
			num_replicas * process_lst_from_leaf_per_replica_time());
	state->lst_received[server_child_partition_index(
			&state->server_state, leaf_lst->leaf_partition_id)] = 1;
	if (lst_received == server_num_children_partitions(&state->server_state) - 1) {
		for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
			state->lst_received[i] = 0;
		}
		for (unsigned int i = 0; i < num_replicas; ++i) {
			set_min(&state->min_lst_vector[i], state->version_vector[i]);
		}
		cpu_add_time(state->cpu,
				num_replicas * process_lst_from_leaf_end_per_replica_time());

		if (state->config->partition == 0) {
			cpu_lock_lock(state->cpu, state->lock_gsv,
					GRV_LST_FROM_LEAF_ROOT_LOCKED, NULL, 0);
		} else {
			grv_send_lst_to_parent(state);
		}
	}
}

void grv_process_lst_from_leaf_root_locked(grv_server_state_t *state)
{
	grv_update_gst_vector(state, state->min_lst_vector);
	cpu_lock_unlock(state->cpu, state->lock_gsv,
			GRV_LST_FROM_LEAF_ROOT_UNLOCKED, NULL, 0);
}

struct grv_send_gst_data {
	grv_server_state_t *state;
	grv_gst_from_root_t *root_gst;
};

static void grv_send_gst(partition_t partition, void *data_)
	{
		struct grv_send_gst_data * data = (struct grv_send_gst_data *) data_;
		grv_server_state_t *state = data->state;
		lpid_t child_lpid = cluster_get_lpid(state->config->cluster,
				state->config->replica, partition);
		server_send(&state->server_state, child_lpid, GRV_GST_FROM_ROOT,
				&data->root_gst->message);
	}

static void grv_send_gst_to_children(grv_server_state_t *state)
{
	unsigned int num_replicas = state->config->cluster->num_replicas;
	grv_gst_from_root_t *root_gst = grv_gst_from_root_new(num_replicas);
	grv_copy_gst_vector(grv_gst_from_root_gst_vector(root_gst), state);
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) root_gst->size);

	struct grv_send_gst_data data = {
		.state = state,
		.root_gst = root_gst,
	};
	server_foreach_children_partition(&state->server_state, grv_send_gst, &data);
	free(root_gst);
}

void grv_process_lst_from_leaf_root_unlocked(grv_server_state_t *state)
{
	// FIXME Currently garbage collection doesn't take cpu time
	// FIXME Garbage collection is currently disabled as it takes too much real time
	//garbage_collect(state);
	grv_send_gst_to_children(state);
}

void grv_send_lst_to_parent(grv_server_state_t *state)
{
	assert(state->config->partition != 0);
	unsigned int num_replicas = state->config->cluster->num_replicas;
	grv_lst_from_leaf_t *leaf_lst = grv_lst_from_leaf_new(num_replicas);
	memcpy(grv_lst_from_leaf_lst_vector(leaf_lst), state->min_lst_vector,
			num_replicas * sizeof(*state->min_lst_vector));
	leaf_lst->leaf_partition_id = state->config->partition;
	for (unsigned int i = 0; i < state->config->tree_fanout; ++i) {
		state->lst_received[i] = 0;
	}
	unsigned int parent_partition = server_parent_partition_id(&state->server_state);
	lpid_t parent_lpid = cluster_get_lpid(state->config->cluster,
			state->config->replica, parent_partition);
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) leaf_lst->simulated_size);
	server_send(&state->server_state, parent_lpid, GRV_LST_FROM_LEAF,
			&leaf_lst->message);
	free(leaf_lst);
}

void grv_schedule_gst_computation_start(grv_server_state_t *state)
{
	server_schedule_self(&state->server_state, gst_interval(),
			GRV_START_GST_COMPUTATION, NULL, 0);
}

void grv_process_gst_from_root(grv_server_state_t *state,
		grv_gst_from_root_t *root_gst)
{
	struct grv_send_gst_data data = {
		.state = state,
		.root_gst = root_gst,
	};
	server_foreach_children_partition(&state->server_state, grv_send_gst,
			&data);
	cpu_lock_lock(state->cpu, state->lock_gsv, GRV_GST_FROM_ROOT_LOCKED,
			root_gst, root_gst->size);
}

void grv_process_gst_from_root_locked(grv_server_state_t *state,
		grv_gst_from_root_t *root_gst)
{
	grv_update_gst_vector(state, grv_gst_from_root_gst_vector(root_gst));
	cpu_lock_unlock(state->cpu, state->lock_gsv, GRV_GST_FROM_ROOT_UNLOCKED,
			root_gst, root_gst->size);
}

void grv_process_gst_from_root_unlocked(grv_server_state_t *state)
{
	cpu_allow_no_time(state->cpu);
	if (server_is_leaf_partition(&state->server_state)) {
		grv_schedule_gst_computation_start(state);
	}
	//FIXME See above FIXME near line 84
	//garbage_collect(state);
}

void grv_process_start_gst_computation(grv_server_state_t *state)
{
	assert(server_is_leaf_partition(&state->server_state));
	if (state->config->partition == 0) {
		// Only one partition
		grv_update_gst_vector(state, state->version_vector);
		grv_schedule_gst_computation_start(state);
		return;
	}
	unsigned int num_replicas = state->config->cluster->num_replicas;
	grv_lst_from_leaf_t *leaf_lst = grv_lst_from_leaf_new(num_replicas);
	grv_copy_version_vector(grv_lst_from_leaf_lst_vector(leaf_lst), state);
	leaf_lst->leaf_partition_id = state->config->partition;
	lpid_t parent_lpid = cluster_get_lpid(
			state->config->cluster, state->config->replica,
			server_parent_partition_id(&state->server_state));
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) leaf_lst->simulated_size);
	server_send(&state->server_state, parent_lpid, GRV_LST_FROM_LEAF,
			&leaf_lst->message);
	free(leaf_lst);
}

int grv_gst_vector_need_update(grv_server_state_t *state, gr_tsp *gst_vector)
{
	for (unsigned int i = 0; i < state->config->cluster->num_replicas; ++i)
	{
		cpu_add_time(state->cpu, check_gst_vector_per_replica_time());
		if (gst_vector[i] > state->gst_vector[i]) {
			return 1;
		}
	}
	return 0;
}

void grv_update_gst_vector(grv_server_state_t *state, gr_tsp *gst_vector)
{
	int increased = 0;

	// Take a copy of the current vector, needed for statistics only
	unsigned int num_replicas = state->config->cluster->num_replicas;
	gr_tsp* old_gst_vector = malloc(num_replicas * sizeof(gr_tsp));
	grv_copy_gst_vector(old_gst_vector, state);

	for (unsigned int i = 0; i < num_replicas; ++i) {
		if (gst_vector[i] > state->gst_vector[i]) {
			state->gst_vector[i] = gst_vector[i];
			increased = 1;
			cpu_add_time(state->cpu, update_gst_vector_per_update_time());
		}
	}
	cpu_add_time(state->cpu, num_replicas * update_gst_vector_per_replica_time());

	if (increased) {
		grv_stats_gst_update(state, old_gst_vector, state->gst_vector);
	}

	free(old_gst_vector);
}

void grv_copy_gst_vector(gr_tsp *dest, grv_server_state_t *state)
{
	memcpy(dest, state->gst_vector,
			state->config->cluster->num_replicas * sizeof(*state->gst_vector));
}

void grv_copy_version_vector(gr_tsp *dest, grv_server_state_t *state)
{
	memcpy(dest, state->version_vector,
			state->config->cluster->num_replicas * sizeof(*state->version_vector));
}
