#include "client/protocols/gr_common.h"
#include "cluster.h"
#include "event.h"

lpid_t client_lpid_for_key(const client_config_t *config, gr_key key) {
	partition_t partition;
	if (config->tied_to_partition) {
		partition = config->partition;
	} else {
		partition = partition_for_key(config->cluster, key);
	}
	return cluster_get_lpid(config->cluster,
			config->replica, partition);
}
