#ifndef client_key_distribution_h
#define client_key_distribution_h

#include "common.h"
#include "client.h"

typedef struct {
	const char *name;
	gr_key (*random_key)(const client_config_t *);
	gr_key (*random_key_on_partition)(const client_config_t *, partition_t);
} client_key_distribution_functions_t;

#ifdef client_key_distribution_c
#define SCLASS
#else
#define SCLASS extern const
#endif
client_key_distribution_functions_t *client_key_distributions;
#undef SCLASS

#endif
