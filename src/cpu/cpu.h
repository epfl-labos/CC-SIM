/* cpu.{c,h}
 *
 * CPU simulation using a queue.
 *
 * The CPU simulation replaces the ProcessEvent() and OnGVT() that an LP
 * previously registered by an implementation that delays the events according
 * to the CPU queue.
 *
 * The ProcessEvent() previously registered by the LP must call cpu_add_time()
 * to record how much simulation time was needed to process an event. The
 * function can be called several time during the processing of an event in
 * which case the times are accumulated.
 */

#ifndef cpu_h
#define cpu_h

typedef struct cpu_state cpu_state_t;
typedef struct cpu_lock cpu_lock_t;

#include "common.h"
#include "cpu/stats.h"
#include <ROOT-Sim.h>

void cpu_add_time(cpu_state_t *state, simtime_t time);
cpu_state_t *cpu_setup(lpid_t lpid, simtime_t now, void *lp_state,
		char *output_prefix, unsigned int busy_cores);

double cpu_elapsed_time(cpu_state_t *state);
void cpu_set_elapsed_time(cpu_state_t *state, double value);

/* Allow the currently processed event to use no CPU time. */
void cpu_allow_no_time(cpu_state_t *state);

// FIXME the lock functions should not return and the following would not be needed
int cpu_lock_called(cpu_state_t *state);

#endif
