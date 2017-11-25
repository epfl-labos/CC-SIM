#ifndef messages_message_h
#define messages_message_h

typedef struct message message_t;

#define MESSAGE_STRUCT(NAME) \
	struct NAME { \
		size_t size; \
		size_t simulated_size; \
	}

MESSAGE_STRUCT(message);

#define MESSAGE_STRUCT_START \
	union { \
		MESSAGE_STRUCT(); \
		message_t message; \
	}

#endif
