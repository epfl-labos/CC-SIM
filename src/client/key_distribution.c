#define client_key_distribution_c
#include "client/key_distribution.h"
#include "cluster.h"

static gr_key random_key_zipf(const client_config_t *config)
{
	return cluster_random_key_zipf(config->cluster,
			config->zipf_skew);
}

static gr_key random_key_zipf_on_partition(const client_config_t *config,
		partition_t partition)
{
	return cluster_random_key_zipf_on_partition(config->cluster,
			partition, config->zipf_skew);
}

static gr_key random_key_uniform(const client_config_t *config)
{
	return cluster_random_key(config->cluster);
}

static gr_key random_key_uniform_on_partition(const client_config_t *config,
		partition_t partition)
{
	return cluster_random_key_on_partition(config->cluster, partition);
}

client_key_distribution_functions_t client_key_distributions_[] = {
	{"uniform", random_key_uniform, random_key_uniform_on_partition},
	{"zipfian", random_key_zipf, random_key_zipf_on_partition},
	{NULL, NULL, NULL},
};

client_key_distribution_functions_t *client_key_distributions =
	&client_key_distributions_[0];
