#ifndef client_gr_common_h
#define client_gr_common_h

#include "client/client.h"

lpid_t client_lpid_for_key(const client_config_t *config, gr_key key);
int client_gr_process_event_common(client_state_t *client, simtime_t now,
		unsigned int event_type, void *data, size_t data_size);

#endif
