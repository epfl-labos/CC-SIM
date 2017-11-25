#ifndef cpu_state_h
#define cpu_state_h

#include "cpu/list.h"
#include "cpu/lock.h"
#include "cpu/rwlock.h"

struct cpu_state {
	lpid_t lpid;
	void (*process_event)(lpid_t, simtime_t, unsigned int, void*, size_t, void*);
	int (*on_gvt)(lpid_t, void*);
	void *lp_state;
	cpu_list_t queue;
	unsigned int cores;
	unsigned int busy_cores;
	simtime_t elapsed_time;
	simtime_t now;
	cpu_stats_t *stats;
	time_t last_output_time;
	char *queue_size_file;
	char *max_queue_size_file;
	int free_core_after_event_processing;
	int allow_no_time;
	int lock_called;
	unsigned int num_locks;
	cpu_lock_t *locks;
	unsigned int num_rwlocks;
	cpu_rwlock_t *rwlocks;
	int continues_in_scheduled_event;
};

int cpu_busy(cpu_state_t *state);
void cpu_busy_cores_dec(cpu_state_t *state);
void cpu_busy_cores_inc(cpu_state_t *state);
void cpu_processing_continues_in_scheduled_event(cpu_state_t *state);

#endif
