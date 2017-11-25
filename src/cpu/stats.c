#include "cpu/stats.h"
#include "cpu/state.h"
#include "common.h"
#include "event.h"
#include "output.h"
#include <assert.h>
#include <errno.h>
#include <json.h>

struct cpu_stats {
	simtime_t start_time;
	simtime_t busy_time;
	unsigned int num_event_types;
	simtime_t *by_event_type_busy_time;
	int *by_event_type_count;
	size_t next_queue_size_sample;
	size_t queue_size_samples_size;
	size_t *queue_size_samples;
	size_t next_max_queue_size_sample;
	size_t max_queue_size_samples_size;
	size_t *max_queue_size_samples;
	size_t max_queue_size;
};

cpu_stats_t *cpu_stats_new(void)
{
	cpu_stats_t *stats = malloc(sizeof(cpu_stats_t));
	stats->busy_time = 0;
	stats->num_event_types = 0;
	stats->by_event_type_busy_time = NULL;
	stats->by_event_type_count = NULL;
	stats->next_queue_size_sample = 0;
	stats->queue_size_samples_size = 0;
	stats->queue_size_samples = NULL;
	stats->next_max_queue_size_sample = 0;
	stats->max_queue_size_samples_size = 0;
	stats->max_queue_size_samples = NULL;
	stats->max_queue_size = 0;
	return stats;
}

void cpu_stats_queue_changed(cpu_state_t *state)
{
	set_max(&state->stats->max_queue_size, state->queue.size);
}

void cpu_stats_update(cpu_state_t *state)
{
	cpu_stats_t *stats = state->stats;
	parray_push(&stats->queue_size_samples, &stats->next_queue_size_sample,
			&stats->queue_size_samples_size, state->queue.size);
	parray_push(&stats->max_queue_size_samples, &stats->next_max_queue_size_sample,
			&stats->max_queue_size_samples_size, stats->max_queue_size);
	stats->max_queue_size = 0;
}

void cpu_stats_flush(cpu_state_t *state)
{
	cpu_stats_t *stats = state->stats;
	FILE *f;

	// FIXME: Check return codes
	f = fopen(state->queue_size_file, "a");
	for (size_t i = 0; i < stats->next_queue_size_sample; ++i) {
		fprintf(f, "%zu\n", stats->queue_size_samples[i]);
	}
	stats->next_queue_size_sample = 0;
	fclose(f);

	// FIXME: Check return codes
	f = fopen(state->max_queue_size_file, "a");
	for (size_t i = 0; i < stats->next_max_queue_size_sample; ++i) {
		fprintf(f, "%zu\n", stats->max_queue_size_samples[i]);
	}
	stats->next_max_queue_size_sample = 0;
	fclose(f);
}

void cpu_stats_event_processed(cpu_state_t *state, unsigned int type)
{
	cpu_stats_t *stats = state->stats;
	stats->busy_time += state->elapsed_time;

	// Update per event type statistics
	if (type >= stats->num_event_types) {
		size_t size = type + 1;
		stats->by_event_type_busy_time = realloc(stats->by_event_type_busy_time,
				size * sizeof(*stats->by_event_type_busy_time));
		stats->by_event_type_count = realloc(stats->by_event_type_count,
				size * sizeof(*stats->by_event_type_count));
		for (unsigned int i = stats->num_event_types; i < size; ++i) {
			stats->by_event_type_busy_time[i] = 0.0;
			stats->by_event_type_count[i] = 0;
		}
		stats->num_event_types = type;
	}
	stats->by_event_type_busy_time[type] += state->elapsed_time;
	stats->by_event_type_count[type]++;
}

void cpu_stats_output(cpu_state_t *state, struct json_object *obj)
{
	assert_gvt();
	assert(json_object_is_type(obj, json_type_object));
	cpu_stats_t *stats = state->stats;
	json_object_object_add(obj, "usage",
			json_object_new_double(stats->busy_time / state->now));

	int event_count = 0;
	for (unsigned int i = 0; i < stats->num_event_types; ++i) {
		event_count += stats->by_event_type_count[i];
	}
	for (unsigned int i = 0; i < stats->num_event_types; ++i) {
		if (stats->by_event_type_busy_time[i] <= 0) continue;
		struct json_object *evt_obj = json_object_new_object();
		json_object_object_add(evt_obj, "usage",
				json_object_new_double(
					stats->by_event_type_busy_time[i] / state->now));
		json_object_object_add(evt_obj, "average time per event",
				json_object_new_double(
					stats->by_event_type_busy_time[i] / stats->by_event_type_count[i]));
		json_object_object_add(evt_obj, "events per second",
				json_object_new_double(
					stats->by_event_type_count[i] / state->now));
		json_object_object_add(evt_obj, "events proportion",
				json_object_new_double(
					stats->by_event_type_count[i] / (double) event_count));
		json_object_object_add(evt_obj, "events",
				json_object_new_double(stats->by_event_type_count[i]));
		json_object_object_add(obj, event_name(i), evt_obj);
	}
}
