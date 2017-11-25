#include "cpu/lock.h"
#include "cpu/cpu.h"
#include "cpu/messages.h"
#include "cpu/list.h"
#include "cpu/schedule.h"
#include "cpu/state.h"
#include "event.h"
#include "parameters.h"
#include <assert.h>

static DEFINE_TIMING_FUNC(lock_time)

struct cpu_lock {
	int locked;
	cpu_list_t *queue;
};

static cpu_lock_t *get_lock(cpu_state_t *state, unsigned int id)
{
	assert(id < state->num_locks);
	return &state->locks[id];
}

cpu_lock_id_t cpu_lock_new(cpu_state_t *state) {
	cpu_lock_id_t id = { .id = state->num_locks };
	cpu_lock_t lock = {
		.locked = 0,
		.queue = cpu_list_new(),
	};
	array_push(&state->locks, &state->num_locks, lock);
	return id;
}

void cpu_lock_lock(cpu_state_t *state, cpu_lock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	assert(event_type != 0);
	cpu_add_time(state, lock_time());
	cpu_schedule_lock_msg(state, CPU_LOCK_LOCK, id.id,
			event_type, data, data_size);
}

static void on_lock_lock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_lock_t *lock = get_lock(state, msg->lock_id);
	if (lock->locked) {
		cpu_list_item_t *item = cpu_list_item_new(CPU_LOCK_LOCK, msg,
				cpu_lock_msg_size(msg));
		cpu_list_push(lock->queue, item);
		cpu_schedule_event(state, 0, CPU_EVENT, NULL, 0);
	} else {
		lock->locked = 1;
		cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
	}
}

void cpu_lock_unlock(cpu_state_t *state, cpu_lock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	assert(get_lock(state, id.id)->locked);
	cpu_add_time(state, lock_time());
	cpu_schedule_lock_msg(state, CPU_LOCK_UNLOCK, id.id,
			event_type, data, data_size);
}

static void on_lock_unlock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_lock_t *lock = get_lock(state, msg->lock_id);
	assert(lock->locked);
	lock->locked = 0;
	if (!cpu_list_empty(lock->queue)) {
		cpu_list_item_t *item = cpu_list_shift(lock->queue);
		cpu_list_unshift(&state->queue, item);
	}
	cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
}

int cpu_lock_process_event(cpu_state_t *state, unsigned int event_type,
		void *data)
{
	switch (event_type) {
		case CPU_LOCK_LOCK:
			on_lock_lock(state, data);
			break;
		case CPU_LOCK_UNLOCK:
			on_lock_unlock(state, data);
			break;
		default:
			return 0;
	}
	return 1;
}
