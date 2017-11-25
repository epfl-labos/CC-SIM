#ifndef messages_base_h
#define messages_base_h

#define MESSAGE_STRUCT(NAME) \
	struct NAME { \
		size_t size; \
		size_t simulated_size; \
	}

#endif
