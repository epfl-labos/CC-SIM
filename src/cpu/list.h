#ifndef cpu_list_h
#define cpu_list_h

#include "cpu/messages.h"

typedef struct cpu_list cpu_list_t;
typedef struct cpu_list_item cpu_list_item_t;

struct cpu_list_item {
	unsigned int event_type;
	size_t data_size;
	void *data;
	struct cpu_list_item *next;
};

struct cpu_list {
	unsigned int size;
	cpu_list_item_t *head;
	cpu_list_item_t *tail;
};

void cpu_list_init(cpu_list_t *list);
cpu_list_t *cpu_list_new(void);
int cpu_list_empty(cpu_list_t *list);

/* Add an item to the tail */
void cpu_list_push(cpu_list_t *list, cpu_list_item_t *item);

/* Remove the head */
cpu_list_item_t *cpu_list_shift(cpu_list_t *list);

/* Add an item at the head */
void cpu_list_unshift(cpu_list_t *list, cpu_list_item_t *item);

/* Remove an item from the tail, currently not needed */
//cpu_list_item_t *cpu_list_pop(cpu_list_t* list);

cpu_list_item_t *cpu_list_item_new(unsigned int event_type, void *data,
		size_t data_size);
cpu_list_item_t *cpu_list_item_new_from_lock_msg(cpu_lock_msg_t *msg);
void cpu_list_item_free(cpu_list_item_t *item);


#endif
