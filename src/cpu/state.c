#include "cpu/state.h"
#include <assert.h>

int cpu_busy(cpu_state_t *state)
{
	return state->busy_cores >= state->cores;
}

void cpu_busy_cores_dec(cpu_state_t *state)
{
	assert(state->busy_cores > 0);
	--state->busy_cores;
}

void cpu_busy_cores_inc(cpu_state_t *state)
{
	assert(state->busy_cores < state->cores);
	++state->busy_cores;
}

void cpu_processing_continues_in_scheduled_event(cpu_state_t *state)
{
	/* Some functions like locking functions requires further processing to be
	 * done in the continuation. It is for instance not possible to acquire two
	 * locks at the same time. Indeed, they must be acquired one after the other. */
	assert(!state->continues_in_scheduled_event);
	state->continues_in_scheduled_event = 1;
}
