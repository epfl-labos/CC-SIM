#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct queue_elem queue_elem_t;

struct queue {
	queue_elem_t *head;
	queue_elem_t *tail;
};

struct queue_elem {
	queue_elem_t *next;
	void *data;
};

queue_t *queue_new(void) {
	queue_t *queue = malloc(sizeof(queue_t));
	queue->head = NULL;
	queue->tail = NULL;
	return queue;
}

void queue_enqueue(queue_t *queue, void *data)
{
	queue_elem_t *elem = malloc(sizeof(queue_elem_t));
	elem->data = data;
	elem->next = NULL;
	if (queue->tail != NULL) {
		queue->tail->next = elem;
	}
	queue->tail = elem;
	if (queue->head == NULL) {
		queue->head = elem;
	}
}

void *queue_dequeue(queue_t *queue)
{
	queue_elem_t *elem = queue->head;
	if (elem == NULL) {
		return NULL;
	}
	void *data = elem->data;
	if (elem->next == NULL) {
		queue->head = NULL;
		queue->tail = NULL;
	} else {
		queue->head = elem->next;
	}
	free(elem);
	return data;
}

void *queue_peek(queue_t *queue)
{
	if (queue->head == NULL)
	{
		return NULL;
	} else {
		return queue->head->data;
	}
}

int queue_is_empty(queue_t *queue)
{
	return queue->head == NULL;
}
