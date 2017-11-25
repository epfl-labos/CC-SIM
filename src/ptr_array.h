#ifndef ptr_array_h
#define ptr_array_h

typedef struct ptr_array ptr_array_t;

struct ptr_array {
	unsigned int size;
	unsigned int allocated_size;
	void **data;
};

void ptr_array_init(ptr_array_t *array);
ptr_array_t *ptr_array_new(void);

/* Return the pointer associated with the given id. */
void *ptr_array_get(ptr_array_t *array, unsigned int id);

/* Set the pointer associated with the given id. */
void ptr_array_set(ptr_array_t *array, unsigned int id, void *ptr);

/* Set the first NULL pointer of the array to the given one and return its id. */
unsigned int ptr_array_put(ptr_array_t *array, void *ptr);

/* Call f for each non-NULL element of the array. */
void ptr_array_foreach(ptr_array_t *array,
		void (*f)(unsigned int id, void *value, void *data), void *data);

#endif
