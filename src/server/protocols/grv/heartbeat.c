#include "server/protocols/grv/heartbeat.h"
#include "common.h"
#include "cpu/cpu.h"
#include "event.h"
#include "gentle_rain.h"
#include "parameters.h"

static DEFINE_PROTOCOL_TIMING_FUNC(process_heartbeat_time, "gr");

void grv_process_heartbeat(grv_server_state_t *state, grv_heartbeat_t *heartbeat)
{
	cpu_lock_lock(state->cpu, state->lock_vv, GRV_HEARTBEAT_LOCKED,
			heartbeat, heartbeat->size);
}

void grv_process_heartbeat_locked(grv_server_state_t *state,
		grv_heartbeat_t *heartbeat)
{
	set_max(&state->version_vector[heartbeat->replica], heartbeat->time);
	cpu_add_time(state->cpu, process_heartbeat_time());
	cpu_lock_unlock(state->cpu, state->lock_vv, CPU_NO_EVENT, NULL, 0);
}

void grv_send_heartbeat(grv_server_state_t *state, int send_time)
{
	grv_heartbeat_t *heartbeat = grv_heartbeat_new();
	heartbeat->replica = state->config->replica;
	heartbeat->time = send_time ? state->clock : 0;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) heartbeat->size);

	void send_heartbeat_message(lpid_t replica_lpid, replica_t replica) {
		if (replica == state->config->replica) return;
		server_send(&state->server_state, replica_lpid, GRV_HEARTBEAT,
				&heartbeat->message);
	}

	foreach_replica(state->config->cluster,
			state->config->partition, send_heartbeat_message);
	free(heartbeat);
}
