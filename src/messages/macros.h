#ifndef messages_macros_h
#define messages_macros_h

#define new_struct_simple(name, additional_size) \
	name##_t *name##_new(void) \
	{ \
		size_t size = sizeof(name##_t); \
		name##_t *ptr = malloc(size); \
		ptr->message.size = size; \
		ptr->message.simulated_size = size + (additional_size); \
		return ptr; \
	}

#define new_struct_with_1_trailer(name, additional_size, \
		count_member, trailing_type, simulated_element_size) \
	name##_t *name##_new(unsigned int count_member) \
	{ \
		size_t size = sizeof(name##_t) + count_member * sizeof(trailing_type); \
		name##_t *ptr = malloc(size); \
		ptr->count_member = count_member; \
		ptr->message.size = size; \
		ptr->message.simulated_size = sizeof(name##_t) \
			+ (additional_size) \
			+ count_member * (simulated_element_size); \
		return ptr; \
	}

#define new_struct_with_2_trailer(name, additional_size, \
		count_member1, trailing_type1, simulated_size1, \
		count_member2, trailing_type2, simulated_size2) \
	name##_t *name##_new(unsigned int count_member1, unsigned int count_member2) \
	{ \
		size_t size = sizeof(name##_t) \
			+ count_member1 * sizeof(trailing_type1) \
			+ count_member2 * sizeof(trailing_type2); \
		name##_t *ptr = malloc(size); \
		ptr->count_member1 = count_member1; \
		ptr->count_member2 = count_member2; \
		ptr->message.size = size; \
		ptr->message.simulated_size = sizeof(name##_t) \
			+ (additional_size) \
			+ count_member1 * (simulated_size1) \
			+ count_member2 * (simulated_size2); \
		return ptr; \
	}

#endif
