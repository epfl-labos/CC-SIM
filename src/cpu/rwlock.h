#ifndef cpu_rwlock_h
#define cpu_rwlock_h

#include "cpu/cpu.h"

typedef struct cpu_rwlock cpu_rwlock_t;

typedef struct {
	unsigned int id;
} cpu_rwlock_id_t;

int cpu_rwlock_process_event(cpu_state_t *state, unsigned int event_type,
		void *data);

cpu_rwlock_id_t cpu_rwlock_new(cpu_state_t *state);
void cpu_rwlock_read_lock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size);
void cpu_rwlock_read_unlock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size);
void cpu_rwlock_write_lock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size);
void cpu_rwlock_write_unlock(cpu_state_t *state, cpu_rwlock_id_t id,
		unsigned int event_type, void *data, size_t data_size);
int cpu_rwlock_write_locked(cpu_state_t *state, cpu_rwlock_id_t id);

#endif
