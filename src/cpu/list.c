#include "cpu/list.h"
#include <assert.h>

void cpu_list_init(cpu_list_t *list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

cpu_list_t *cpu_list_new(void)
{
	cpu_list_t *list = malloc(sizeof(cpu_list_t));
	cpu_list_init(list);
	return list;
}

int cpu_list_empty(cpu_list_t *list)
{
	return list->head == NULL;
}

void cpu_list_push(cpu_list_t *list, cpu_list_item_t *item)
{
	if (list->tail == NULL) {
		list->head = item;
	} else {
		list->tail->next = item;
	}
	list->tail = item;
	list->size++;
}

cpu_list_item_t *cpu_list_shift(cpu_list_t *state)
{
	cpu_list_item_t *item = state->head;
	if (item == NULL) return NULL;
	state->head = item->next;
	if (state->tail == item) state->tail = NULL;
	state->size--;
	item->next = NULL;
	return item;
}

void cpu_list_unshift(cpu_list_t *list, cpu_list_item_t *item)
{
	if (list->head == NULL) {
		list->tail = item;
		item->next = NULL;
	} else {
		item->next = list->head;
	}
	list->head = item;
	list->size++;
}

cpu_list_item_t *cpu_list_item_new(unsigned int event_type,
		void *data, size_t data_size)
{
	cpu_list_item_t *item = malloc(sizeof(cpu_list_item_t));
	item->event_type = event_type;
	item->data = malloc(data_size);
	memcpy(item->data, data, data_size);
	item->data_size = data_size;
	item->next = NULL;
	return item;
}

cpu_list_item_t *cpu_list_item_new_from_lock_msg(cpu_lock_msg_t *msg)
{
	return cpu_list_item_new(msg->event_type, cpu_lock_msg_data(msg),
			msg->data_size);
}

void cpu_list_item_free(cpu_list_item_t *item)
{
	free(item->data);
	free(item);
}
