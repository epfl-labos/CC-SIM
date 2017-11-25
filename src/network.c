#include "network.h"
#include "parameters.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <json.h>
#include <limits.h>

#define DISCONNECTED -1


/* FIXME: There's currently no receive queue only a sending queue
 * This means congestion at the receiver is not simulated. */

struct network_config {
	unsigned int num_lps;
	double *transmission_rates;
	simtime_t **network_delay;
};

struct network_state {
	network_config_t *conf;
	simtime_t busy_until;
	simtime_t busy_time;
	simtime_t last_reception_time_by_lp[MAX_LP];
};

void network_send(network_state_t *state, lpid_t from_lpid, lpid_t to_lpid,
		simtime_t now, unsigned int event_type, void *data, size_t data_size,
		size_t simulated_size)
{
	network_config_t *conf = state->conf;
	assert(from_lpid < conf->num_lps);
	assert(to_lpid < conf->num_lps);

	simtime_t propagation_time = conf->network_delay[from_lpid][to_lpid];
	assert(propagation_time != DISCONNECTED);
	assert(propagation_time >= 0);
	simtime_t transmission_rate = conf->transmission_rates[from_lpid];
	assert(transmission_rate > 0);

	simtime_t transmission_time = (simtime_t) simulated_size / transmission_rate;
	assert(transmission_time >= 0);

	// Update the time until which the interface is busy transmitting this message
	if (state->busy_until < now) state->busy_until = now;
	state->busy_until += transmission_time;
	simtime_t when = state->busy_until + propagation_time;

	// Ensure the message arrives after the previous one sent to the same
	// destination.
	assert(when >= state->last_reception_time_by_lp[to_lpid]);
	state->last_reception_time_by_lp[to_lpid] = when;

	assert(data_size <= UINT_MAX);
	ScheduleNewEvent(to_lpid, when, event_type, data, (unsigned int) data_size);

	/* Statistics, keep track how long the interface was busy */
	if (now >= app_params.ignore_initial_seconds) {
		state->busy_time += transmission_time;
	}
}

network_config_t *network_setup(unsigned int num_lps)
{
	void *__real_malloc(size_t size);
	network_config_t *conf = __real_malloc(sizeof(network_config_t));
	conf->num_lps = num_lps;
	conf->network_delay = __real_malloc(num_lps * sizeof(simtime_t*));
	conf->transmission_rates = __real_malloc(num_lps * sizeof(double));
	for (unsigned int i = 0; i < num_lps; ++i) {
		conf->transmission_rates[i] = 0;
		conf->network_delay[i] = __real_malloc(num_lps * sizeof(simtime_t));
		for (unsigned int j = 0; j < num_lps; ++j) {
			conf->network_delay[i][j] = DISCONNECTED;
		}
	}
	return conf;
}

network_state_t *network_init(network_config_t *conf)
{
	network_state_t *state = malloc(sizeof(network_state_t));
	state->conf = conf;
	state->busy_until = 0;
	state->busy_time = 0;
	memset(state->last_reception_time_by_lp, 0,
			MAX_LP * sizeof(*state->last_reception_time_by_lp));
	return state;
}

void network_set_delay(network_config_t *conf, lpid_t from_lp, lpid_t to_lp,
		simtime_t delay)
{
	conf->network_delay[from_lp][to_lp] = delay;
}

void network_set_transmission_rate(network_config_t *conf, lpid_t lpid,
		double rate)
{
	assert(lpid < conf->num_lps);
	assert(rate > 0);
	conf->transmission_rates[lpid] = rate / 8;
}

void network_stats_output(network_state_t *state, struct json_object *obj, simtime_t now)
{
	json_object_object_add(obj, "usage", json_object_new_double(
			state->busy_time / (now - app_params.ignore_initial_seconds)));
}
