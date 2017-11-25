#include "common.h"
#include <ROOT-Sim.h>
#include <assert.h>
#include <limits.h>

void randomize_value(uint8_t *value_ptr)
{
	*value_ptr = (uint8_t) RandomRange(0, 255);
}

unsigned int random_uint(unsigned int min, unsigned int max) {
	assert(min < max);
	assert(max <= INT_MAX);
	return (unsigned int) RandomRange((int) min, (int) max);
}

char *mallocstrcy(const char* s)
{
	char *new = malloc(strlen(s) + 1);
	strcpy(new, s);
	return new;
}
