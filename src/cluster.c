#include "cluster.h"
#include "common.h"
#include "parameters.h"
#include <assert.h>
#include <limits.h>
#include <ROOT-Sim.h>

#define partition_index(replica, partition) \
	(replica * cluster->num_partitions + partition)

cluster_config_t *cluster_new(allocator alloc, unsigned int num_replicas,
		unsigned int num_partitions, unsigned int num_keys, double clock_skew)
{
	cluster_config_t *cluster = alloc(sizeof(cluster_config_t));
	cluster->num_replicas = num_replicas;
	cluster->num_partitions = num_partitions;
	cluster->server_lpids = alloc(
			num_replicas * num_partitions * sizeof(*cluster->server_lpids));
	cluster->key_min = 0;
	cluster->key_max = num_keys;
	assert(clock_skew >= 0);
	cluster->clock_skew = clock_skew;
	return cluster;
}

void cluster_set_lpid(cluster_config_t *cluster, replica_t replica,
		partition_t partition, lpid_t lpid)
{
	cluster->server_lpids[partition_index(replica, partition)] = lpid;
}

lpid_t cluster_get_lpid(cluster_config_t *cluster, replica_t replica,
		partition_t partition)
{
	assert(replica < cluster->num_replicas);
	assert(partition < cluster->num_partitions);
	return cluster->server_lpids[partition_index(replica, partition)];
}

partition_t partition_for_key(cluster_config_t *cluster, gr_key key)
{
	return (partition_t) (key % cluster->num_partitions);
}

gr_key cluster_random_key(cluster_config_t *cluster)
{
	if (cluster->key_max == UINT_MAX) {
		return (gr_key) RandomRange(INT_MIN, INT_MAX);
	} else {
		assert(cluster->key_max <= INT_MAX);
		return (gr_key) RandomRange(0, (int) cluster->key_max);
	}
}

gr_key cluster_random_key_on_partition(cluster_config_t *cluster,
		partition_t partition)
{
	assert(partition < cluster->num_partitions);
	gr_key key = cluster_random_key(cluster);
	partition_t p = partition_for_key(cluster, key);

	if (p == partition) return key;
	partition_t delta;
	if (p < partition) {
		delta = partition - p;
	} else {
		delta = cluster->num_partitions - p + partition;
	}

	gr_key new_key = key + delta;
	if (new_key > key) {
		if (new_key <= cluster->key_max) {
			key = new_key;
		} else {
			key = partition;
		}
	} else {
		key = partition;
	}

	assert(partition == partition_for_key(cluster, key));
	return key;
}

gr_key cluster_random_key_zipf(cluster_config_t *cluster, double skew)
{
	assert(cluster->key_max <= INT_MAX);
	return (gr_key) Zipf(skew, (int) cluster->key_max);
}

gr_key cluster_random_key_zipf_on_partition(cluster_config_t *cluster,
		partition_t partition, double skew)
{
	// If the number of keys is not divisible by the number of partitions, a
	// small number of keys have a zero probability of being picked.
	gr_key keys_per_partition = cluster->key_max / cluster->num_partitions;
	assert(keys_per_partition < INT_MAX);
	return partition * keys_per_partition + (gr_key) Zipf(skew, (int) keys_per_partition);
}

void foreach_server(cluster_config_t *cluster,
		void(*f)(lpid_t, replica_t, partition_t))
{
	for (replica_t r = 0; r < cluster->num_replicas; ++r) {
		for (partition_t p = 0; p < cluster->num_partitions; ++p) {
			f(cluster->server_lpids[partition_index(r, p)], r, p);
		}
	}
}

void foreach_partition(cluster_config_t *cluster, replica_t replica,
		void(*f)(lpid_t, partition_t))
{
	for (partition_t p = 0; p < cluster->num_partitions; ++p) {
		f(cluster->server_lpids[partition_index(replica, p)], p);
	}
}

void foreach_replica(cluster_config_t *cluster, partition_t partition,
		void(*f)(lpid_t, replica_t))
{
	for (replica_t r = 0; r < cluster->num_replicas; ++r) {
		f(cluster->server_lpids[partition_index(r, partition)], r);
	}
}
