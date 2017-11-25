#define application_c
#include "client/client.h"
#include "cluster.h"
#include "common.h"
#include "event.h"
#include "network.h"
#include "output.h"
#include "parameters.h"
#include "server/server.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct event {
	int source_lp;
} event_t;

// To be able to allocate memory before starting the ROOT Sim main loop
void *__real_malloc(size_t size);

static lpid_t num_lps = 0;
static char * lp_names[MAX_LP];
static void (*process_event_callbacks[MAX_LP])(lpid_t, simtime_t, unsigned int, void*, size_t, void*);
static int (*on_gvt_callbacks[MAX_LP])(lpid_t, void*);

static lpid_t new_process(const char *name) {
	if (num_lps >= MAX_LP) {
		fprintf(stderr, "Couldn't allocate a new LP:"
				"the maximum of %d has been reached\n", MAX_LP);
		exit(1);
	}
	size_t name_length = strlen(name);
	char *buf = __real_malloc(name_length + 1);
	strncpy(buf, name, name_length);
	buf[name_length] = '\0';
	lp_names[num_lps] = buf;
	return num_lps++;
}

const char* lp_name(lpid_t lpid)
{
	return lp_names[lpid];
}

void register_callbacks(lpid_t lpid,
		void (*process_event)(lpid_t, simtime_t, unsigned int, void*, size_t, void*),
		int (*on_gvt)(lpid_t, void*))
{
	process_event_callbacks[lpid] = process_event;
	on_gvt_callbacks[lpid] = on_gvt;
}

void get_callbacks(
		void (**process_event)(lpid_t, simtime_t, unsigned int, void*, size_t, void*),
		int (**on_gvt)(lpid_t, void*),
		lpid_t lpid)
{
	*process_event = process_event_callbacks[lpid];
	*on_gvt = on_gvt_callbacks[lpid];
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
// This will trigger -Wmissing-prototypes as it is substituted to an undeclared
// function by a ROOT-Sim macro.
void ProcessEvent(lpid_t lpid, simtime_t now, unsigned int event_type, void *data, size_t data_size, void *state)
{
	//fprintf(stderr, "[%0.10f] lpid %d, type %d (%s), data %p, size %d, state %p\n",
	//		now, lpid, event_type, event_name(event_type), data, data_size, state);
	process_event_callbacks[lpid](lpid, now, event_type, data, data_size, state);
	return;
}
#pragma GCC diagnostic pop

int processing_gvt = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
// This will trigger -Wmissing-prototypes as it is substituted to an undeclared
// function by a ROOT-Sim macro.
int OnGVT(lpid_t lpid, void *snapshot)
{
	processing_gvt = 1;
	int ret =  on_gvt_callbacks[lpid](lpid, snapshot);
	processing_gvt = 0;
	return ret;
}
#pragma GCC diagnostic pop

static const char *output_dir = NULL;
static const char *config_file = NULL;
struct app_parameters app_params;

int override_stop_after = 0;
double stop_after;
int override_stop_after_real_time = 0;
time_t stop_after_real_time;

static int parse_arguments(int argc, char **argv)
{
	int options_index = 0;
	static struct option long_options[] = {
		// Order is important, see "case 0" below
		{"stop-after", required_argument, 0, 0},
		{"output-dir", required_argument, 0, 0},
		{"verbose", no_argument, 0, 0},
		{"quiet", no_argument, 0, 0},
		{"config", required_argument, 0, 0},
		{"stop-after-real-time", required_argument, 0, 0},
		{0, 0, 0, 0}
	};

#define PARSE_UINT_ARG(var) \
	if (optarg) { \
		int i = atoi(optarg); \
		if (i < 0) { \
			fprintf(stderr, "Argument must be positive\n"); \
			exit(1); \
		} \
		var = (unsigned int) i; \
	} else { \
		fprintf(stderr, "Missing integer argument after --%s option\n", \
				long_options[options_index].name); \
		exit(1); \
	}

#define PARSE_DOUBLE_ARG(var) \
	if (optarg) { \
		var = atof(optarg); \
	} else { \
		fprintf(stderr, "Missing double argument after --%s option\n", \
				long_options[options_index].name); \
		exit(1); \
	}

	while (1) {
		int c = getopt_long(argc, argv, "", long_options, &options_index);

		if (c == -1) break;
		switch (c) {
			case 0:
				switch (options_index) {
					case 0:
						PARSE_DOUBLE_ARG(stop_after);
						override_stop_after = 1;
						break;
					case 1: output_dir = optarg; break;
					case 2: verbose = 1; break;
					case 3: verbose = 0; break;
					case 4: config_file = optarg; break;
					case 5:
						PARSE_UINT_ARG(stop_after_real_time);
						stop_after_real_time += time(NULL);
						override_stop_after = 1;
						break;
				}
		}
	}

	// Reset getopt_long state
	options_index = optind;
	optind = 0;

	if (verbose == 2 && output_dir == NULL) verbose = 1;

	return options_index;
#undef PARSE_INT_ARG
}

int main(int argc, char **argv)
{
	int parsed_arguments =  parse_arguments(argc, argv);
	output_init(output_dir);
	if (config_file == NULL) {
		fprintf(stderr, "A configuration file must be specified using"
				" the --config argument.\n");
		exit(1);
	}
	parameters_read_from_file(config_file, &app_params);
	if (override_stop_after) {
		app_params.stop_after_simulated_seconds = stop_after;
	}
	if (override_stop_after_real_time) {
		app_params.stop_after_real_time = stop_after_real_time;
	}
	output_log_startup(argc, argv);

	const size_t buf_size = 128;
	char buf[buf_size];

	/* Cluster */
	struct json_object *cluster_obj = param_get_object_root("cluster");
	unsigned int num_partitions_per_replica = param_get_uint(
			cluster_obj, "partitions_per_replica");
	unsigned int num_clients_per_partition = param_get_uint(
			cluster_obj, "clients_per_partition");
	unsigned int num_replicas = param_get_uint(cluster_obj, "replicas");
	unsigned int num_keys = param_get_uint(cluster_obj, "keys");
	double clock_skew = param_get_double(cluster_obj, "clock_skew");
	unsigned int num_cores = param_get_uint(cluster_obj, "cores_per_server");
	cluster_config_t *cluster = cluster_new(
			__real_malloc, num_replicas, num_partitions_per_replica,
			num_keys, clock_skew);

	/* Network */
	lpid_t _num_lps = num_partitions_per_replica * num_replicas
		* (1 + num_clients_per_partition);
	network_config_t *network = network_setup(_num_lps);

	/* Partitions */
	unsigned int tree_fanout = param_get_uint(cluster_obj, "tree_fanout");
	void setup_server(lpid_t lpid, replica_t replica, partition_t partition) {
		snprintf(buf, buf_size, "server %d of replica %d", partition, replica);
		lpid = new_process(buf);
		cluster_set_lpid(cluster, replica, partition, lpid);
		server_setup(lpid, cluster, replica, partition, network, tree_fanout,
				num_cores);
	}
	foreach_server(cluster, setup_server);

	/* Clients */
	struct json_object *client_obj = param_get_object_root("client");
	const char *workload = param_get_string(client_obj, "workload");
	const char *key_distribution = param_get_string(client_obj, "key distribution");
	double zipf_skew = param_get_double(client_obj, "zipfian skew");
	for (replica_t r = 0; r < num_replicas; ++r) {
		for (partition_t p = 0; p < num_partitions_per_replica; ++p) {
			for (unsigned int c = 0; c < num_clients_per_partition; ++c) {
				snprintf(buf, buf_size,
						"client %d of partition %d in replica %d", c, p, r);
				lpid_t client_lpid = new_process(buf);
				client_setup(client_lpid, cluster, r, p, network, workload,
						key_distribution, zipf_skew);
			}
		}
	}

	assert(num_lps == _num_lps);

	// Build argv for rootsim
	int remaining_argc = argc - parsed_arguments;
	int rootsim_argc = remaining_argc + 3;
	char *rootsim_argv[rootsim_argc + 1];
	rootsim_argv[rootsim_argc] = NULL;
	rootsim_argv[0] = argv[0];

	// Copy the remaining arguments
	memcpy(rootsim_argv + 1, argv + parsed_arguments,
			(size_t) remaining_argc * sizeof(char*));

	// Add --nprc
	const char * nprc = "--nprc";
	rootsim_argv[rootsim_argc - 2] = __real_malloc(strlen(nprc) + 1);
	strcpy(rootsim_argv[rootsim_argc - 2], nprc);
	snprintf(buf, buf_size, "%d", num_lps);
	rootsim_argv[rootsim_argc - 1] = buf;

	output_log_rootsim_args(rootsim_argc, rootsim_argv);
	output_log_lps(num_lps);
	printf("Starting ROOT-Sim main loop (--nprc argument is overridden by this application)...\n");
	if (argc > 1) {
		rootsim_main(rootsim_argc, rootsim_argv);
	} else {
		// Show ROOT-Sim help message
		rootsim_main(argc, argv);
	}
}
