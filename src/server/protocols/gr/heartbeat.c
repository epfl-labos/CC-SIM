#include "server/protocols/gr/heartbeat.h"
#include "common.h"
#include "cpu/cpu.h"
#include "event.h"
#include "gentle_rain.h"
#include "parameters.h"

static DEFINE_PROTOCOL_TIMING_FUNC(process_heartbeat_time, "gr");

static size_t gr_heartbeat_size(gr_heartbeat_t *heartbeat)
{
	(void) heartbeat;
	return sizeof(gr_heartbeat_t);
}

void gr_process_heartbeat(gr_server_state_t *state, gr_heartbeat_t *heartbeat)
{
	cpu_add_time(state->cpu, process_heartbeat_time());
	if (state->version_vector[heartbeat->replica] < heartbeat->time) {
		cpu_lock_lock(state->cpu, state->replica_locks[heartbeat->replica],
				GR_HEARTBEAT_LOCKED, heartbeat, gr_heartbeat_size(heartbeat));
	}
}

void gr_process_heartbeat_locked(gr_server_state_t *state, gr_heartbeat_t *heartbeat)
{
	cpu_add_time(state->cpu, process_heartbeat_time());
	if (state->version_vector[heartbeat->replica] < heartbeat->time) {
		state->version_vector[heartbeat->replica] = heartbeat->time;
	}
	cpu_lock_unlock(state->cpu, state->replica_locks[heartbeat->replica],
			CPU_NO_EVENT, NULL, 0);
}

void gr_send_heartbeat(gr_server_state_t *state, int send_time)
{
	gr_heartbeat_t *heartbeat = gr_heartbeat_new();
	heartbeat->replica = state->config->replica;
	heartbeat->time = send_time ? state->clock : 0;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) heartbeat->simulated_size);

	void send_heartbeat_message(lpid_t replica_lpid, replica_t replica) {
		if (replica == state->config->replica) return;
		server_send(&state->server_state, replica_lpid, GR_HEARTBEAT,
				&heartbeat->message);
	}
	foreach_replica(state->config->cluster,
			state->config->partition, send_heartbeat_message);

	free(heartbeat);
}
