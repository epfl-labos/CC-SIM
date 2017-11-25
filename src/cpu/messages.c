#include "cpu/messages.h"
#include "cpu/cpu.h"
#include "cpu/state.h"

void *cpu_lock_msg_data(cpu_lock_msg_t *msg)
{
	return msg + 1;
}

cpu_lock_msg_t *cpu_lock_msg_new(size_t *size_ptr, unsigned int id,
		unsigned int event_type, void *data, size_t data_size)
{
	*size_ptr = sizeof(cpu_lock_msg_t) + data_size;
	cpu_lock_msg_t *msg = malloc(*size_ptr);
	msg->lock_id = id;
	msg->event_type = event_type;
	msg->data_size = data_size;
	memcpy(cpu_lock_msg_data(msg), data, data_size);
	return msg;
}

size_t cpu_lock_msg_size(cpu_lock_msg_t *msg)
{
	return sizeof(cpu_lock_msg_t) + msg->data_size;
}
