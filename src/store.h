/* store.{c,h}
 *
 * Actual store were the key-value pairs are stored. There's nothing
 * simulation- or Gentle Rain-specific in here.
 */

#ifndef store_h
#define store_h

#include "gentle_rain.h"

typedef struct store store_t;

store_t *store_new(void);
void *store_get(store_t *store, gr_key key);
void store_put(store_t *store, gr_key key, void *value);
void store_foreach_item(store_t *store, void (*fun)(gr_key key, void *value));


#endif
