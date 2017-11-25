#ifndef cpu_lock_h
#define cpu_lock_h

#include "cpu/cpu.h"

typedef struct cpu_lock cpu_lock_t;

typedef struct {
	unsigned int id;
} cpu_lock_id_t;


int cpu_lock_process_event(cpu_state_t *state, unsigned int event_type,
		void *data);

cpu_lock_id_t cpu_lock_new(cpu_state_t *state);
void cpu_lock_lock(cpu_state_t *state, cpu_lock_id_t id,
		unsigned int event_type, void *data, size_t data_size);
void cpu_lock_unlock(cpu_state_t *state, cpu_lock_id_t id,
		unsigned int event_type, void *data, size_t data_size);

#endif
