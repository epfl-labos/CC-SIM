#include "server/stats.h"
#include "output.h"
#include "parameters.h"
#include "server.h"
#include <assert.h>
#include <json.h>

#define average_stat(stats, name, average_ptr) \
	do { \
		*average_ptr = 0; \
		if (stats->name##_index == 0) break; \
		for (unsigned int i = 0; i < stats->name##_index; ++i) { \
			*average_ptr += stats->name[i]; \
		} \
		*average_ptr = (double) *average_ptr / (double) stats->name##_index; \
	} while (0);

#define clear_stat(stats, name) \
	stats->name##_index = 0;

typedef unsigned int server_stats_array_id_t;

typedef struct {
	char *name;
	double *values;
	unsigned int next_index;
	unsigned int size;
} stat_array_t;

typedef struct {
	char * name;
	int value;
} stat_counter_t;

struct server_stats {
	unsigned int num_arrays;
	stat_array_t **arrays;
	unsigned int num_counters;
	stat_counter_t **counters;
};

static server_stats_array_id_t stats_array_new(server_stats_t *stats,
		const char *name)
{
	server_stats_array_id_t id = stats->num_arrays;
	stat_array_t *array = calloc(1, sizeof(stat_array_t));
	array->name = mallocstrcy(name);
	array_push(&stats->arrays, &stats->num_arrays, array);
	return id;
}

static double stats_array_average(stat_array_t *array)
{
	double average = 0;
	if (array->next_index == 0) return 0;
	for (unsigned int i = 0; i < array->next_index; ++i) {
		average += array->values[i];
	}
	return average / (double) array->next_index; \
}

static server_stats_counter_id_t stats_counter_new(server_stats_t *stats,
		const char *name)
{
	server_stats_counter_id_t id = stats->num_counters;
	stat_counter_t *counter = calloc(1, sizeof(stat_counter_t));
	counter->name = mallocstrcy(name);
	array_push(&stats->counters, &stats->num_counters, counter);
	return id;
}

server_stats_t *server_stats_new(void)
{
	server_stats_t *stats = calloc(1, sizeof(server_stats_t));

	/* Define a few well-known stats */
	server_stats_array_id_t id;
	id = stats_array_new(stats, "replication time");
	assert(id == REPLICATION_TIME);
	id = stats_array_new(stats, "value staleness");
	assert(id == VALUE_STALENESS);
	id = stats_array_new(stats, "visibility latency");
	assert(id == VISIBILITY_LATENCY);
	id = stats_counter_new(stats, "get requests");
	assert(id == GET_REQUESTS);
	id = stats_counter_new(stats, "put requests");
	assert(id == PUT_REQUESTS);
	id = stats_counter_new(stats, "rotx requests");
	assert(id == ROTX_REQUESTS);
	id = stats_counter_new(stats, "replica updates");
	assert(id == REPLICA_UPDATES);

	return stats;
}

server_stats_array_id_t server_stats_array_new(server_state_t *state,
		const char *name)
{
	return stats_array_new(state->stats, name);
}

void server_stats_array_push(server_state_t* state,
		server_stats_array_id_t id, double value)
{
	assert(id < state->stats->num_arrays);
	if (state->now < app_params.ignore_initial_seconds) return;
	stat_array_t *stat = state->stats->arrays[id];
	parray_push(&stat->values, &stat->next_index, &stat->size, value);
}

server_stats_counter_id_t server_stats_counter_new(server_state_t *state,
		const char *name)
{
	return stats_counter_new(state->stats, name);
}

void server_stats_counter_inc(server_state_t *state,
		server_stats_counter_id_t id)
{
	assert(id < state->stats->num_counters);
	if (state->now < app_params.ignore_initial_seconds) return;
	++state->stats->counters[id]->value;
}

void server_stats_output(server_state_t *state, struct json_object *top)
{
	assert_gvt();
	assert(json_object_is_type(top, json_type_object));
	server_stats_t *stats = state->stats;
	simtime_t elapsed = state->now - app_params.ignore_initial_seconds;

	// Counters
	for (unsigned int i = 0; i < stats->num_counters; ++i) {
		struct json_object *obj = json_object_new_object();
		json_object_object_add(obj, "count",
				json_object_new_int64(stats->counters[i]->value));
		json_object_object_add(obj, "rate",
				json_object_new_double(stats->counters[i]->value / elapsed));
		json_object_object_add(top, stats->counters[i]->name, obj);
	}

	// Arrays
	for (unsigned int i = 0; i < stats->num_arrays; ++i) {
		struct json_object *obj = json_object_new_object();
		json_object_object_add(obj, "sample size",
				json_object_new_int64(stats->arrays[i]->next_index));
		json_object_object_add(obj, "average",
				json_object_new_double(stats_array_average(stats->arrays[i])));
		json_object_object_add(top, stats->arrays[i]->name, obj);
	}

	// FIXME This might be useful to do it for every array
	// Output each visibility latency sample
	const size_t buf_size = 32;
	char buf[buf_size];
	snprintf(buf, buf_size, "_visibility");
	FILE *f_visibility = output_server_open_suffix(state->config->replica,
			state->config->partition, buf);
	if (f_visibility != NULL) {
		for (unsigned int i = 0; i < stats->arrays[VISIBILITY_LATENCY]->next_index; ++i) {
			fprintf(f_visibility, "%f\n",
					stats->arrays[VISIBILITY_LATENCY]->values[i]);
		}
		fclose(f_visibility);
	}
}
