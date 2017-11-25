#include "cpu/schedule.h"
#include "cpu/messages.h"
#include "cpu/list.h"
#include "cpu/schedule.h"
#include "cpu/state.h"
#include "event.h"
#include <assert.h>
#include <limits.h>

void cpu_process(cpu_state_t *state,
		unsigned int type, void *data, size_t data_size)
{
	if (type == CPU_NO_EVENT) {
		cpu_process_next_in_queue(state);
		return;
	}

	cpu_busy_cores_inc(state);
	state->elapsed_time = 0;
	state->lock_called = 0;
	state->allow_no_time = 0;
	state->free_core_after_event_processing = 1;

	if (cpu_lock_process_event(state, type, data)
			|| cpu_rwlock_process_event(state, type, data)) {
		state->free_core_after_event_processing = 0;
	} else {
		state->process_event(state->lpid, state->now, type, data, data_size,
				state->lp_state);
	}

	if (!state->allow_no_time) {
		assert(state->elapsed_time > 0);
	}

	cpu_stats_event_processed(state, type);
	if (state->free_core_after_event_processing) {
		cpu_schedule_event(state, state->elapsed_time, CPU_FREE_CORE, NULL, 0);
	}
}

void cpu_process_next_in_queue(cpu_state_t *state)
{
	cpu_list_item_t *item = cpu_list_shift(&state->queue);
	if (item != NULL) {
		cpu_process(state, item->event_type, item->data, item->data_size);
		cpu_list_item_free(item);
	}
}

void cpu_schedule_event(cpu_state_t *state, simtime_t delay,
		unsigned int event_type, void *data, size_t data_size)
{
	assert(data_size < UINT_MAX);
	ScheduleNewEvent(state->lpid, state->now + delay, event_type,
			data, (unsigned int) data_size);
}

void cpu_schedule_lock_msg(cpu_state_t *state, unsigned int cpu_event_type,
		unsigned int id, unsigned int event_type, void *data, size_t data_size)
{
	cpu_processing_continues_in_scheduled_event(state);
	state->free_core_after_event_processing = 0;
	size_t size;
	cpu_lock_msg_t *msg = cpu_lock_msg_new(&size, id, event_type, data, data_size);
	cpu_schedule_event(state, state->elapsed_time, cpu_event_type, msg, size);
	free(msg);
}
