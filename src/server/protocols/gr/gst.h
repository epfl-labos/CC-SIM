/* server_gst.{c,h}
 *
 * Handle the computation of the GST according to the Gentle Rain algorithm.
 */

#ifndef server_gst_h
#define server_gst_h

#include "messages/gr.h"
#include "server/protocols/gr/gr.h"

void gr_process_lst_from_leaf(gr_server_state_t *state, gr_lst_from_leaf_t *leaf_lst);
void gr_process_lst_from_leaf_root_locked(gr_server_state_t *state);
void gr_process_lst_from_leaf_root_unlocked(gr_server_state_t *state);
void gr_process_gst_from_root(gr_server_state_t *state, gr_gst_from_root_t  *root_gst);
void gr_process_gst_from_root_locked(gr_server_state_t *state,
		gr_gst_from_root_t *root_gst);
void gr_process_gst_from_root_unlocked(gr_server_state_t *state,
		gr_gst_from_root_t *root_gst);
void gr_process_start_gst_computation(gr_server_state_t *state);
void gr_schedule_gst_computation_start(gr_server_state_t *state);
void gr_send_gst_to_children(gr_server_state_t *state);
void gr_send_lst_to_parent(gr_server_state_t *state);
int gr_gst_need_update(gr_server_state_t *state, gr_tsp gst);
void gr_update_gst(gr_server_state_t *state, gr_tsp gst);

#endif
