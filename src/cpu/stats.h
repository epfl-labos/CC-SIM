#ifndef cpu_stats_h
#define cpu_stats_h

typedef struct cpu_stats cpu_stats_t;

#include "cpu.h"
#include <json.h>

cpu_stats_t *cpu_stats_new(void);
void cpu_stats_queue_changed(cpu_state_t *state);
void cpu_stats_update(cpu_state_t *state);
void cpu_stats_flush(cpu_state_t *state);
void cpu_stats_event_processed(cpu_state_t *state, unsigned int type);
void cpu_stats_output(cpu_state_t *state, struct json_object *obj);

#endif
