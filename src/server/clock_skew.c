#include "server/clock_skew.h"
#include <math.h>

void server_clock_skew_init(server_clock_skew_state_t *state,
		cluster_config_t *cluster)
{
	state->offset = fabs(Normal()) * cluster->clock_skew;
}

simtime_t server_clock_skew(server_clock_skew_state_t *state,
	simtime_t universal_time)
{
	return universal_time + state->offset;
}
