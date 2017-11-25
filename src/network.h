/* network.{c,h}
 *
 * Network simulation. The transmission time and propagation delay are simulated.
 */

#ifndef network_h
#define network_h

#include "common.h"
#include <json.h>
#include <ROOT-Sim.h>

typedef struct network_config network_config_t;
typedef struct network_state network_state_t;

network_config_t *network_setup(unsigned int num_lps);
network_state_t *network_init(network_config_t *conf);
void network_set_delay(network_config_t *state, lpid_t from_lp, lpid_t to_lp,
		simtime_t delay);

/* Set the transmission rate (in bit/s) of the network adapter of lpid */
void network_set_transmission_rate(network_config_t *state, lpid_t lpid,
		double rate);

void network_send(network_state_t *state, lpid_t from_lpid, lpid_t to_lpid,
		simtime_t now, unsigned int event_type, void *data, size_t data_size,
		size_t simulated_size);
void network_stats_output(network_state_t *state, struct json_object *obj, simtime_t now);

#endif
