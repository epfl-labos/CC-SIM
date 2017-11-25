#include "cpu/cpu.h"
#include "cpu/lock.h"
#include "cpu/list.h"
#include "cpu/rwlock.h"
#include "cpu/schedule.h"
#include "cpu/state.h"
#include "cpu/stats.h"
#include "event.h"
#include "parameters.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <time.h>

struct cpu_lock {
	int locked;
	cpu_list_item_t *head;
	cpu_list_item_t *tail;
	void *locked_by;
};

typedef struct {
	unsigned int lock_id;
} cpu_unlock_msg_t;

static void cpu_process_event(lpid_t lpid, simtime_t now, unsigned int event_type,
		void *data, size_t data_size, void *_state)
{
	cpu_state_t *state = (cpu_state_t*) _state;
	state->now = now;
	state->continues_in_scheduled_event = 0;

	switch (event_type) {
		case CPU_FREE_CORE:
			cpu_busy_cores_dec(state);
			cpu_process_next_in_queue(state);
			break;
		case CPU_EVENT:
			assert(!cpu_busy(state));
			cpu_process_next_in_queue(state);
			break;
		case CPU_STATS:
			cpu_stats_update(state);
			ScheduleNewEvent(lpid, now + cpu_stats_interval, CPU_STATS, NULL, 0);
			break;
		default: {
			int handled = cpu_lock_process_event(state, event_type, data)
				|| cpu_rwlock_process_event(state, event_type, data);
			if (!handled) {
				if (cpu_busy(state)) {
					cpu_list_item_t *item = cpu_list_item_new(
							event_type, data, data_size);
					cpu_list_push(&state->queue, item);
				} else {
					cpu_process(state, event_type, data, data_size);
				}
			}
		}
	}
}

static int cpu_on_gvt(lpid_t lpid, void *_state)
{
	cpu_state_t *state = (cpu_state_t*) _state;
	int ret = state->on_gvt(lpid, state->lp_state);
	time_t now = time(NULL);
	if (ret || state->last_output_time < now - 60) {
		cpu_stats_flush(state);
		state->last_output_time = now;
	}
	return ret;
}

void cpu_add_time(cpu_state_t *state, simtime_t time)
{
	assert(time >= 0);
	assert(!state->lock_called);
	state->elapsed_time += time;
}

double cpu_elapsed_time(cpu_state_t *state)
{
	return state->elapsed_time;
}

void cpu_set_elapsed_time(cpu_state_t *state, double value)
{
	state->elapsed_time = value;
}

void cpu_allow_no_time(cpu_state_t *state)
{
	state->allow_no_time = 1;
}

int cpu_lock_called(cpu_state_t *state)
{
	return state->lock_called;
}

cpu_state_t *cpu_setup(lpid_t lpid, simtime_t now, void *lp_state,
		char *output_prefix, unsigned int cores)
{
	cpu_state_t *state = malloc(sizeof(cpu_state_t));
	state->lpid = lpid;
	state->stats = cpu_stats_new();
	state->lp_state = lp_state;
	cpu_list_init(&state->queue);
	state->cores = cores;
	state->busy_cores = 0;
	state->elapsed_time = 0;
	state->num_locks = 0;
	state->locks = NULL;
	state->num_locks = 0;
	state->locks = NULL;
	state->num_rwlocks = 0;
	state->rwlocks = NULL;
	state->lock_called = 0;
	state->last_output_time = 0;
	char name[PATH_MAX];
	size_t output_prefix_length = strlen(output_prefix);
	strncpy(name, output_prefix, PATH_MAX);
	strncat(name, "_cpu_stats.json", PATH_MAX);
	name[output_prefix_length] = '\0';
	strncat(name, "_cpu_queue_size", PATH_MAX);
	state->queue_size_file = mallocstrcy(name);
	name[output_prefix_length] = '\0';
	strncat(name, "_cpu_max_queue_size", PATH_MAX);
	state->max_queue_size_file = mallocstrcy(name);
	get_callbacks(&state->process_event, &state->on_gvt, lpid);
	register_callbacks(lpid, cpu_process_event, cpu_on_gvt);
	SetState(state);
	ScheduleNewEvent(lpid, now + cpu_stats_interval, CPU_STATS, NULL, 0);
	return state;
}
