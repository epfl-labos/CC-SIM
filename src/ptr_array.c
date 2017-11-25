#include "ptr_array.h"
#include <assert.h>
#include <stdlib.h>

#define INITIAL_ARRAY_SIZE 4

void ptr_array_init(ptr_array_t *array)
{
	array->size = 0;
	array->allocated_size = 0;
	array->data = NULL;
}
ptr_array_t *ptr_array_new(void);

void *ptr_array_get(ptr_array_t *array, unsigned int id)
{
	assert(id < array->size);
	return array->data[id];
}

void ptr_array_set(ptr_array_t *array, unsigned int id, void *ptr)
{
	assert(id < array->size);
	array->data[id] = ptr;
}

unsigned int ptr_array_put(ptr_array_t *array, void *ptr)
{
	assert(ptr != NULL);
	for (unsigned int i = 0; i < array->size; ++i) {
		if (array->data[i] == NULL) {
			array->data[i] = ptr;
			return i;
		}
	}
	if (array->size == array->allocated_size) {
		if (array->allocated_size == 0) {
			array->allocated_size = INITIAL_ARRAY_SIZE;
		} else {
			array->allocated_size *= 2;
		}
		array->data = realloc(array->data, array->allocated_size * sizeof(void*));
	}
	unsigned int id = array->size;
	++array->size;
	array->data[id] = ptr;
	return id;
}

void ptr_array_foreach(ptr_array_t *array,
		void (*f)(unsigned int id, void *value, void *data), void *data)
{
	for (unsigned int i = 0; i < array->size; ++i) {
		if (array->data[i] != NULL) {
			f(i, array->data[i], data);
		}
	}
}
