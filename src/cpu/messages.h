#ifndef cpu_messages_h
#define cpu_messages_h

#include "cpu/cpu.h"

typedef struct {
	unsigned int lock_id;
	unsigned int event_type;
	size_t data_size;
} cpu_lock_msg_t;

void *cpu_lock_msg_data(cpu_lock_msg_t *msg);
cpu_lock_msg_t *cpu_lock_msg_new(size_t *size_ptr, unsigned int id,
		unsigned int event_type, void *data, size_t data_size);
size_t cpu_lock_msg_size(cpu_lock_msg_t *msg);

#endif
