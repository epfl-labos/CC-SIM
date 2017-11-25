#ifndef server_clock_skew_h
#define server_clock_skew_h

#include "common.h"
#include "cluster.h"

typedef struct {
	simtime_t offset;
} server_clock_skew_state_t;

void server_clock_skew_init(server_clock_skew_state_t *state,
		cluster_config_t *cluster);
simtime_t server_clock_skew(server_clock_skew_state_t *state,
		simtime_t universal_time) __attribute__ ((warn_unused_result));

#endif
