#include "server/protocols/gr/slice.h"
#include "common.h"
#include "event.h"
#include "parameters.h"
#include "server/protocols/gr/gst.h"
#include "server/protocols/gr/rotx.h"
#include "server/protocols/gr/snapshot.h"
#include "server/protocols/gr/store.h"
#include <assert.h>

static DEFINE_PROTOCOL_TIMING_FUNC(process_slice_response_per_value_time, "gr");

static int is_value_visible_snapshot(gr_server_state_t *state, item_t *item,
		gr_tsp time)
{
	(void) state; // Unused parameter
	return item->update_time <= time;
}

void gr_process_slice_request(gr_server_state_t *state, gr_slice_request_t *request)
{
	if (gr_gst_need_update(state, request->snapshot_time)) {
		cpu_lock_lock(state->cpu, state->lock, GR_SLICE_REQUEST_LOCKED,
				request, request->simulated_size);
	} else {
		gr_process_slice_request_unlocked(state, request);
	}
}

void gr_process_slice_request_locked(gr_server_state_t *state,
		gr_slice_request_t *request)
{
	gr_update_gst(state, request->snapshot_time);
	cpu_lock_unlock(state->cpu, state->lock, GR_SLICE_REQUEST_UNLOCKED,
			request, request->simulated_size);
}

void gr_process_slice_request_unlocked(gr_server_state_t *state,
		gr_slice_request_t *request)
{
	gr_slice_response_t *response = gr_slice_response_new();
	gr_get_value_at(&response->value, &response->update_time, NULL, state,
			request->key, request->snapshot_time, is_value_visible_snapshot);
	response->gst = state->gst;
	response->snapshot_id = request->snapshot_id;
	response->key_id = request->key_id;
	cpu_add_time(state->cpu, build_struct_per_byte_time()
			* (simtime_t) response->simulated_size);
	server_send(&state->server_state, request->from_lpid,
			GR_SLICE_RESPONSE, &response->message);
}

void gr_process_slice_response(gr_server_state_t *state,
		gr_slice_response_t *slice)
{
	if (gr_gst_need_update(state, slice->gst)) {
		cpu_lock_lock(state->cpu, state->lock, GR_SLICE_RESPONSE_LOCKED,
				slice, slice->size);
	} else {
		gr_process_slice_response_unlocked(state, slice);
	}
}

void gr_process_slice_response_locked(gr_server_state_t *state,
		gr_slice_response_t *slice)
{
	gr_update_gst(state, slice->gst);
	cpu_lock_unlock(state->cpu, state->lock, GR_SLICE_RESPONSE_UNLOCKED,
			slice, slice->size);
}

void gr_process_slice_response_unlocked(gr_server_state_t *state,
		gr_slice_response_t *slice)
{
	gr_snapshot_state_t *snapshot =
		gr_snapshot_state_get(state, slice->snapshot_id);
	snapshot->values[slice->key_id] = slice->value;
	set_max(&snapshot->update_time, slice->update_time);
	set_max(&snapshot->gst, slice->gst);
	++snapshot->received_values;
	cpu_add_time(state->cpu, process_slice_response_per_value_time());
	if (snapshot->received_values == snapshot->size) {
			gr_send_snapshot_response(state, slice->snapshot_id);
	}
}
