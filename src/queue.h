#ifndef queue_h
#define queue_h

typedef struct queue queue_t;

queue_t *queue_new(void);
void queue_enqueue(queue_t *queue, void *data);
void *queue_dequeue(queue_t *queue);
void *queue_peek(queue_t *queue);
int queue_is_empty(queue_t *queue);

#endif
