#pragma once
#define _GNU_SOURCE
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

struct dynamic_array {
	int size;
	int capacity;
	size_t item_size;
	char *data;
};

#define DYNAMIC_ARRAY_INIT ((struct dynamic_array) {0, 0, 0, NULL})

static inline void *da_item(struct dynamic_array *da, int n)
{

	return (n >= 0 && n < da->size) ? da->data + n * da->item_size : NULL;
}

static inline void *da_expand(struct dynamic_array *da, size_t item_size, int n)
{
	int cap = da->capacity;

	if (da->item_size == 0)
		da->item_size = item_size;
	else
		assert(da->item_size == item_size && "inconsistent array item size");

	while (da->size + n > cap) {
		if (cap)
			cap = cap << 1;
		else
			cap = da->size + n > 8 ? da->size + n : 8;
	}

	if (cap > da->capacity) {
		da->capacity = cap;
		if (da->data)
			da->data = realloc(da->data, da->item_size * cap);
		else
			da->data = malloc(da->item_size * cap);
	}

	return da->data + da->item_size * da->size;
}

#define da_append(da, item) do {			\
	typeof(item) *__next;				\
	__next = da_expand(da, sizeof(*__next), 1);	\
	*__next = (item);				\
	(da)->size++;					\
} while (0)


static inline void *da_pop(struct dynamic_array *da)
{
	void *item = da_item(da, da->size - 1);

	if (da->size > 0)
		da->size--;

	return item;
}

static inline void da_free(struct dynamic_array *da)
{
	if (da->capacity > 0)
		free(da->data);

	*da = DYNAMIC_ARRAY_INIT;
}
