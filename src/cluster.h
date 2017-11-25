/* cluster.{h,c}
 *
 * Cluster configuration and related functions.
 */

#ifndef cluster_h
#define cluster_h

#include "common.h"
#include "gentle_rain.h"

typedef struct cluster_config {
	unsigned int num_replicas;
	unsigned int num_partitions;
	lpid_t *server_lpids;
	gr_key key_min;
	gr_key key_max;
	double clock_skew;
} cluster_config_t;

cluster_config_t *cluster_new(allocator alloc, unsigned int num_replicas,
		unsigned int num_partitions, unsigned int num_keys, double clock_skew);
void cluster_set_lpid(cluster_config_t *cluster, replica_t replica,
		partition_t partition, lpid_t lpid);
lpid_t cluster_get_lpid(cluster_config_t *cluster, replica_t replica,
		partition_t partition);
partition_t partition_for_key(cluster_config_t *cluster, gr_key key);
gr_key cluster_random_key(cluster_config_t *cluster);
gr_key cluster_random_key_on_partition(cluster_config_t *cluster,
		partition_t partition);
gr_key cluster_random_key_zipf(cluster_config_t *cluster, double skew);
gr_key cluster_random_key_zipf_on_partition(cluster_config_t *cluster,
		partition_t partition, double skew);
gr_value random_value(void);
void foreach_server(cluster_config_t *cluster, void(*f)(lpid_t, replica_t, partition_t));
void foreach_partition(cluster_config_t *cluster, replica_t replica,
		void (*f)(lpid_t, partition_t));
void foreach_replica(cluster_config_t *cluster, partition_t partiton,
		void (*f)(lpid_t, replica_t));

#endif
