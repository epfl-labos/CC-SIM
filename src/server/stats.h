/* server_stats.c
 *
 * Functions to record and output server-related statistics.
 *
 * Reminder: It is unsafe to output anything outside of an OnGVT() call.
 */

#ifndef server_stats_h
#define server_stats_h

typedef struct server_stats server_stats_t;

#include "common.h"
#include "server.h"
#include <json.h>

server_stats_t *server_stats_new(void);
void server_stats_output(server_state_t *state, struct json_object *obj);

/** .. c:type:: server_stats_array_id_t
 *
 *  The id of an array. A new one can be obtained through
 *  :c:func:`server_stats_array_new`.
 *
 *  .. c:var:: REPLICATION_TIME
 *
 *      The id of the array where the replication time of each value is pushed.
 *
 *  .. c:var:: VALUE_STALENESS
 *
 *      The id of the array where the value staleness of each value returned to
 *      a client is pushed.
 *
 *  .. c:var:: VISIBILITY_LATENCY
 *
 *      The id of the array where the visibility latency of each value is pushed.
 */
typedef unsigned int server_stats_array_id_t;
#define REPLICATION_TIME   0
#define VALUE_STALENESS    1
#define VISIBILITY_LATENCY 2

/** .. c:type:: server_stats_counter_id_t
 *
 *  The id of a counter. A new one can be obtained through
 *  :c:func:`server_stats_counter_new`.
 *
 *  .. c:var:: GET_REQUESTS
 *
 *      The id of the counter tracking the number of get requests processed by
 *      the server.
 *
 *  .. c:var:: PUT_REQUESTS
 *
 *      The id of the counter tracking the number of put requests processed by
 *      the server.
 *
 *  .. c:var:: ROTX_REQUESTS
 *
 *      The id of the counter tracking the number of rotx requests processed by
 *      the server.
 *
 *  .. c:var:: REPLICA_UPDATES
 *
 *      The id of the counter tracking the number of updates from replica
 *      processed by the server.
 */
typedef unsigned int server_stats_counter_id_t;
#define GET_REQUESTS    0
#define PUT_REQUESTS    1
#define ROTX_REQUESTS   2
#define REPLICA_UPDATES 3

/** .. c:function:: server_stats_array_id_t server_stats_array_new(server_state_t*, const char* name)
 *
 *  Create a new array. The average and number of samples will be found in the
 *  simulation output with the given name. Use the returned id with
 *  :c:func:`server_stats_array_push` to add values to the array.
 */
server_stats_array_id_t server_stats_array_new(server_state_t*, const char* name);

/** .. c:function:: void server_stats_array_push(server_state_t*, server_stats_array_id_t, double)
 *
 *  Append a value to the given array. Use :c:func:`server_stats_array_new` to
 *  create a new array.
 */
void server_stats_array_push(server_state_t*, server_stats_array_id_t, double);

/** .. c:function:: server_stats_counter_id_t server_stats_counter_new(server_state_t*, const char *name)
 *
 *  Create a new counter. The count and the average rate at which it is
 *  incremented will be found in the simulation output with the given name. Use
 *  the returned id with :c:func:`server_stats_counter_inc` to increment the
 *  counter.
 */
server_stats_counter_id_t server_stats_counter_new(server_state_t*, const char *name);

/** .. c:function:: void server_stats_counter_inc(server_state_t*, server_stats_counter_id_t)
 *
 *  Increment the given counter. Use :c:func:`server_stats_counter_new` to
 *  create a new counter.
 */
void server_stats_counter_inc(server_state_t*, server_stats_counter_id_t);

#endif
