#ifndef cpu_schedule_h
#define cpu_schedule_h

#include "cpu/list.h"
#include "cpu/state.h"

void cpu_process(cpu_state_t *state, unsigned int type, void *data,
		size_t data_size);
void cpu_process_next_in_queue(cpu_state_t *state);
void cpu_schedule_event(cpu_state_t *state, simtime_t delay,
		unsigned int event_type, void *data, size_t data_size);
void cpu_schedule_lock_msg(cpu_state_t *state, unsigned int cpu_event_type,
		unsigned int id, unsigned int event_type, void *data, size_t data_size);

#endif
