/* common.{c,h}
 *
 * Utility functions and macros.
 */

#ifndef common_h
#define common_h

#include <ROOT-Sim.h>
#include <stdint.h>

#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

typedef void*(*allocator)(size_t);
typedef unsigned int lpid_t;
typedef unsigned int partition_t;
typedef unsigned int replica_t;

/* Keys and values */
typedef double gr_tsp;
typedef uint8_t gr_value;
typedef uint64_t gr_key; // Unfortunately key_t is defined in sys/types.h
size_t simulated_value_size; // Defined in application.c
void randomize_value(gr_value *value_ptr);

// Currently defined in core/core.h in ROOT-Sim sources but is not public
#define MAX_LP 65536

/* Global variables defined in application.c */
#ifdef application_c
#define SCLASS
#else
#define SCLASS extern const
#endif
SCLASS struct app_parameters app_params;
SCLASS int verbose;
SCLASS void *lp_config[MAX_LP];
SCLASS int processing_gvt;
#undef SCLASS

const char* lp_name(lpid_t lpid);

void register_callbacks(
		lpid_t lpid,
		void (*process_event)(lpid_t, simtime_t, unsigned int, void*, size_t, void*),
		int (*on_gvt)(lpid_t, void*));

void get_callbacks(
		void (**process_event)(lpid_t, simtime_t, unsigned int, void*, size_t, void*),
		int (**on_gvt)(lpid_t, void*),
		lpid_t lpid);

#define set_max(store_ptr, value) \
	if (*store_ptr < value) { \
		*store_ptr = value; \
	}

#define set_min(store_ptr, value) \
	if (*store_ptr > value) { \
		*store_ptr = value; \
	}

#define inc_range(value, min, max) \
	(value + 1 > max ? min : value + 1)

unsigned int random_uint(unsigned int min, unsigned int max);

#define INITIAL_ARRAY_SIZE 8

/* Reallocate an array increasing it's size by one and write the value in the new slot. */
#define array_push(array_ptr, current_size_ptr, value) \
	do { \
		unsigned int new_size = *(current_size_ptr) + 1; \
		*(array_ptr) = realloc(*(array_ptr), sizeof(**(array_ptr)) * new_size); \
		(*(array_ptr))[*(current_size_ptr)] = value; \
		*(current_size_ptr) = new_size; \
	} while (0)

#define parray_push(array_ptr, index_ptr, size_ptr, value) \
	do { \
		if (*(index_ptr) == *(size_ptr)) { \
			*(size_ptr) = *(size_ptr) ? *(size_ptr) * 2 : INITIAL_ARRAY_SIZE; \
			*(array_ptr) = realloc(*(array_ptr), sizeof(**(array_ptr)) * *(size_ptr)); \
		} \
		(*(array_ptr))[*(index_ptr)] = value; \
		(*(index_ptr))++; \
	} while (0)

#define double_array_as_needed(array_ptr, size_ptr, wanted_index) \
	if (wanted_index >= *size_ptr) { \
		if (*size_ptr == 0) *size_ptr = 16; \
		while (*size_ptr <= wanted_index) *size_ptr *= 2; \
		*(array_ptr) = realloc(*(array_ptr), *size_ptr * sizeof(**(array_ptr))); \
	}

#define max_array(max_ptr, array, size) \
	if (size > 0) { \
		*max_ptr = array[0]; \
		for (typeof(size) i = 1; i < size; ++i) { \
			set_max(max_ptr, array[i]); \
		} \
	} \

#endif

#define assert_gvt() \
	assert(processing_gvt && "This function can be called from on GVT only.")

char *mallocstrcy(const char* s);
