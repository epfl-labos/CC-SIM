#include <stdlib.h>
#include "store.h"

typedef struct store_item store_item_t;

struct store {
	unsigned int num_buckets;
	store_item_t **buckets;
};

struct store_item {
	gr_key key;
	void *value;
	store_item_t *next;
};

static unsigned int hash(store_t *store, gr_key key)
{
	return (unsigned int) (key % (store->num_buckets));
}

store_t *store_new(void)
{
	const unsigned int buckets_per_store = 32768;
	store_t *store = malloc(sizeof(store_t));
	store->num_buckets = buckets_per_store;
	store->buckets = calloc(buckets_per_store, sizeof(store_item_t*));
	return store;
}

void *store_get(store_t *store, gr_key key)
{
	store_item_t *item = store->buckets[hash(store, key)];
	while (item != NULL && item->key != key) {
		item = item->next;
	}

	if (item == NULL) return NULL;
	return item->value;
}

void store_put(store_t *store, gr_key key, void *value)
{
	store_item_t **item = &store->buckets[hash(store, key)];
	while (*item != NULL) {
		if ((*item)->key == key) {
			(*item)->value = value;
			return;
		}
		item = &(*item)->next;
	}
	store_item_t *new_item = malloc(sizeof(store_item_t));
	new_item->key = key;
	new_item->value = value;
	new_item->next = NULL;
	*item = new_item;
}

void store_foreach_item(store_t *store, void (*fun)(gr_key key, void *value))
{
	for (unsigned int i = 0; i < store->num_buckets; ++i) {
		store_item_t *item = store->buckets[i];
		while (item != NULL) {
			fun(item->key, item->value);
			item = item->next;
		}
	}
}
