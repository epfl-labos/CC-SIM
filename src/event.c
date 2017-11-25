#define event_c
#include "event.h"

#define ENUM_ITEM_NAME(i) #i,

const char* event_type_names[] = {
	FOREACH_EVENT(ENUM_ITEM_NAME)
};

const char* event_name(event_type_t type) {
	return event_type_names[type];
}
