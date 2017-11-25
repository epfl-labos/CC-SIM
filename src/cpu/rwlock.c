#include "cpu/rwlock.h"
#include "cpu/cpu.h"
#include "cpu/messages.h"
#include "cpu/list.h"
#include "cpu/schedule.h"
#include "cpu/state.h"
#include "event.h"
#include "parameters.h"
#include <assert.h>

static DEFINE_TIMING_FUNC(lock_time)

// The counter tracks the number of readers
// A reader has to wait when (write_lock && counter == 0)
// A writer has to wait when (write_lock)

struct cpu_rwlock {
	int counter;
	int write_locked;
	cpu_list_t *queue;
};

static cpu_rwlock_t *get_rwlock(cpu_state_t *state, unsigned int id)
{
	assert(id < state->num_rwlocks);
	return &state->rwlocks[id];
}

cpu_rwlock_id_t cpu_rwlock_new(cpu_state_t *state)
{
	cpu_rwlock_id_t id = { .id = state->num_rwlocks };
	cpu_rwlock_t rwlock = {
		.counter = 0,
		.write_locked = 0,
		.queue = cpu_list_new(),
	};
	array_push(&state->rwlocks, &state->num_rwlocks, rwlock);
	return id;
}

void cpu_rwlock_read_lock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	cpu_add_time(state, lock_time());
	cpu_schedule_lock_msg(state, CPU_RWLOCK_READ_LOCK, id.id,
			event_type, data, data_size);
}

static void on_rwlock_read_lock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_rwlock_t *rwlock = get_rwlock(state, msg->lock_id);
	if (rwlock->write_locked && rwlock->counter == 0) {
		assert(msg->event_type != 0);
		cpu_list_item_t *item = cpu_list_item_new(CPU_RWLOCK_READ_LOCK,
				msg, cpu_lock_msg_size(msg));
		cpu_list_push(rwlock->queue, item);
		cpu_schedule_event(state, 0, CPU_EVENT, NULL, 0);
	} else {
		++rwlock->counter;
		if (rwlock->counter == 1) {
			rwlock->write_locked = 1;
		}
		cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
	}
}

void cpu_rwlock_read_unlock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	cpu_add_time(state, lock_time());
	cpu_schedule_lock_msg(state, CPU_RWLOCK_READ_UNLOCK, id.id,
			event_type, data, data_size);
}

static void on_rwlock_read_unlock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_rwlock_t *rwlock = get_rwlock(state, msg->lock_id);
	assert(rwlock->counter > 0);
	--rwlock->counter;
	if (rwlock->counter == 0) {
		assert(rwlock->write_locked);
		rwlock->write_locked = 0;
		if (!cpu_list_empty(rwlock->queue)) {
			cpu_list_item_t *item = cpu_list_shift(rwlock->queue);
			cpu_list_unshift(&state->queue, item);
		}
	}
	cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
}

void cpu_rwlock_write_lock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	cpu_add_time(state, lock_time());
	cpu_schedule_lock_msg(state, CPU_RWLOCK_WRITE_LOCK, id.id,
			event_type, data, data_size);
}

static void on_rwlock_write_lock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_rwlock_t *rwlock = get_rwlock(state, msg->lock_id);
	if (rwlock->write_locked) {
		cpu_list_item_t *item = cpu_list_item_new(CPU_RWLOCK_WRITE_LOCK,
				msg, cpu_lock_msg_size(msg));
		cpu_list_push(rwlock->queue, item);
		cpu_schedule_event(state, 0, CPU_EVENT, NULL, 0);
	} else {
		rwlock->write_locked = 1;
		cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
	}
}

void cpu_rwlock_write_unlock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size)
{
	cpu_add_time(state, lock_time());
	assert(get_rwlock(state, id.id)->write_locked);
	cpu_schedule_lock_msg(state, CPU_RWLOCK_WRITE_UNLOCK, id.id,
			event_type, data, data_size);
}

static void on_rwlock_write_unlock(cpu_state_t *state, cpu_lock_msg_t *msg)
{
	cpu_add_time(state, lock_time());
	cpu_busy_cores_dec(state);
	cpu_rwlock_t *rwlock = get_rwlock(state, msg->lock_id);
	assert(rwlock->counter == 0 || !cpu_list_empty(rwlock->queue));
	assert(rwlock->write_locked);
	rwlock->write_locked = 0;
	if (!cpu_list_empty(rwlock->queue)) {
		cpu_list_item_t *item = cpu_list_shift(rwlock->queue);
		cpu_list_unshift(&state->queue, item);
	}
	cpu_process(state, msg->event_type, cpu_lock_msg_data(msg), msg->data_size);
}

int cpu_rwlock_write_locked(cpu_state_t *state, cpu_rwlock_id_t id)
{
	cpu_rwlock_t *rwlock = get_rwlock(state, id.id);
	return rwlock->write_locked;
}

int cpu_rwlock_process_event(cpu_state_t *state, unsigned int event_type,
		void *data)
{
	switch (event_type) {
		case CPU_RWLOCK_READ_LOCK:
			on_rwlock_read_lock(state, data);
			break;
		case CPU_RWLOCK_READ_UNLOCK:
			on_rwlock_read_unlock(state, data);
			break;
		case CPU_RWLOCK_WRITE_LOCK:
			on_rwlock_write_lock(state, data);
			break;
		case CPU_RWLOCK_WRITE_UNLOCK:
			on_rwlock_write_unlock(state, data);
			break;
		default:
			return 0;
	}
	return 1;
}
