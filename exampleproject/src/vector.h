#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>

typedef struct vector {
	size_t size;
	size_t count;
	size_t elem_size;
	void *elems;
} vector;

void vector_init(vector *vec, size_t elem_size);
void vector_free(vector *vec);
void vector_alloc(vector *vec, size_t count);
void vector_set(vector *vec, size_t idx, void *elem);
void *vector_get(vector *vec, size_t idx);

#endif
