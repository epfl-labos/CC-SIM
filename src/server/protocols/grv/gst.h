#ifndef server_protocols_grv_gst_h
#define server_protocols_grv_gst_h

#include "messages/grv.h"
#include "server/protocols/grv/grv.h"

void grv_process_gst_from_root(grv_server_state_t *state,
		grv_gst_from_root_t  *root_gst);
void grv_process_gst_from_root_locked(grv_server_state_t *state,
		grv_gst_from_root_t *root_gst);
void grv_process_gst_from_root_unlocked(grv_server_state_t *state);
void grv_process_lst_from_leaf(grv_server_state_t *state,
		grv_lst_from_leaf_t *leaf_lst);
void grv_process_lst_from_leaf_root_locked(grv_server_state_t *state);
void grv_process_lst_from_leaf_root_unlocked(grv_server_state_t *state);
void grv_process_start_gst_computation(grv_server_state_t *state);
void grv_schedule_gst_computation_start(grv_server_state_t *state);
void grv_send_lst_to_parent(grv_server_state_t *state);
int grv_gst_vector_need_update(grv_server_state_t *state, gr_tsp *gst_vector);
void grv_update_gst_vector(grv_server_state_t *state, gr_tsp *gst_vector);
void grv_copy_gst_vector(gr_tsp *dest, grv_server_state_t *state);
void grv_copy_version_vector(gr_tsp *dest, grv_server_state_t *state);

#endif
