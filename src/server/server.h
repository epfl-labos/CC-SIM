/* server.{c,h}
 *
 * Server initialization and dispatching of events sent to a server to the
 * correct process_* functions defined in the various server_*.c files.
 */

#ifndef server_h
#define server_h

typedef struct server_state server_state_t;

#include "cluster.h"
#include "cpu/cpu.h"
#include "messages/message.h"
#include "network.h"
#include "server/clock_skew.h"
#include "server/stats.h"
#include "store.h"

/** .. c:type:: server_config_t
 *
 *  .. c:member:: cluster_config_t *cluster
 *
 *  The configuration of the cluster the server is part of.
 *
 *  .. c:member:: network_config_t *network
 *
 *  The configuration of the network.
 *
 *  .. c:member:: lpid_t lpid
 *
 *  The id of the associated ROOT-Sim LP. Each server has a unique LP associated with it.
 *
 *  .. c:member:: partition_t partition
 *
 *  The partition id of the server.
 *
 *  .. c:member:: replica_t replica
 *
 *  The id of the replica the server is part of.
 *
 *  .. c:member:: int tree_fanout
 *
 *  The fanout of the tree into which the partitions are arranged.
 */
typedef struct server_config {
	cluster_config_t *cluster;
	network_config_t *network;
	lpid_t lpid;
	partition_t partition;
	replica_t replica; // The replica the server belongs to
	unsigned int tree_fanout;
	unsigned int num_cores;
} server_config_t;

/** .. c:type:: server_state_t
 *
 *  The state of the server common to all protocols.
 *
 *  .. c:member:: start_time
 *
 *  The simulation time at which the server started.
 *
 *  .. c:member:: clock
 *
 *  The value of the physical clock.
 *
 *  .. c:member:: now
 *
 *  The current simulation time (as oposed to
 *  :c:member:`clock` this value is perfectly synchronised).
 *
 *  .. c:member:: finished
 *
 *  Whether the simulation should stop and the outputs have already been written.
 *
 *  .. c:member:: config
 *
 *  The configuration (:c:type:`server_config_t`) of the server.
 *
 *  .. c:member:: store
 *
 *  The store (:c:type:`store_t`) where the server stores the key-value pairs.
 *
 *  .. c:member:: stats
 *
 *  An instance of :c:type:`server_stats_t` for storing statistics of the server.
 *
 *  .. c:member:: cpu
 *
 *  An instance of :c:type:`cpu_state_t` simulating the CPU of the server.
 *
 *  .. c:member:: network
 *
 *  An instance of :c:type:`network_state_t` storing
 *  network-related state and statistics.
 */

#define SERVER_STATE_STRUCT(NAME) \
	struct NAME { \
		gr_tsp clock; \
		simtime_t now; \
		simtime_t start_time; \
		int finished; \
		const server_config_t *config; \
		store_t *store; \
		server_stats_t *stats; \
		cpu_state_t *cpu; \
		network_state_t *network; \
		server_clock_skew_state_t clock_skew; \
	}

SERVER_STATE_STRUCT(server_state);

void server_setup(lpid_t lpid, cluster_config_t *cluster, replica_t replica,
		partition_t partition, network_config_t *network,
		unsigned int tree_fanout, unsigned int num_cores);

/** .. c:function:: void server_schedule_self(server_state_t *state, simtime_t delay, unsigned int event_type, void *data, size_t data_size)
 *
 *  Schedule an event to oneself. The message will be delivered after the
 *  specified delay elapses. The ownership of `data` remains at
 *  the caller (a copy is made for the receiver).
 */
void server_schedule_self(server_state_t *state, simtime_t delay,
		unsigned int event_type, void *data, size_t data_size);

/** .. c:function:: void server_send(server_state_t *state, lpid_t to_lpid, unsigned int event_type, message_t *message)
 *
 *  Send an event to another server or client. The time needed to perform the
 *  operation is automatically accumulated. The ownership of `message` remains at
 *  the caller (a copy is made for the receiver).
 */
void server_send(server_state_t *state, lpid_t to_lpid,
		unsigned int event_type, message_t *message);

unsigned int server_parent_partition_id(server_state_t *state);
unsigned int server_child_partition_index(server_state_t *state,
		unsigned int partition_id);
unsigned int server_first_child_id(server_state_t *state);
unsigned int server_last_child_id(server_state_t *state);
int server_is_leaf_partition(server_state_t *state);
unsigned int server_num_children_partitions(server_state_t *state);
void server_foreach_children_partition(server_state_t *state,
		void (*f)(partition_t partition, void *data), void *data);

#endif
