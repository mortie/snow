#include "vector.h"
#include <string.h>

void vector_init(vector *vec, size_t elem_size)
{
	vec->size = 0;
	vec->count = 0;
	vec->elem_size = elem_size;
	vec->elems = NULL;
}

void vector_free(vector *vec)
{
	free(vec->elems);
}

void vector_alloc(vector *vec, size_t count)
{
	if (vec->size >= count)
		return;

	if (vec->size == 0)
		vec->size = 16;
	while (vec->size < count)
		vec->size *= 2;

	vec->elems = realloc(vec->elems, vec->elem_size * vec->size);
}

void vector_set(vector *vec, size_t idx, void *elem)
{
	if (idx + 1 > vec->count)
	{
		vec->count = idx + 1;
		vector_alloc(vec, vec->count);
	}

	void *ptr = (void *)((char *)vec->elems + (vec->elem_size * idx));
	memcpy(ptr, elem, vec->elem_size);
}

void *vector_get(vector *vec, size_t idx)
{
	if (vec->count <= idx)
		return NULL;

	void *ptr = (void *)((char *)vec->elems + (vec->elem_size * idx));
	return ptr;
}

#include <snow/snow.h>

describe(vector) {
	vector vec;

	it("breaks the rules of math (to demonstrate failed tests)") {
		assert(1 == 2, "Oh noes!");
	}

	after_each() {
		vector_free(&vec);
	}

	it("inits vectors correctly") {
		vector_init(&vec, 53);

		asserteq(vec.size, 0);
		asserteq(vec.count, 0);
		asserteq(vec.elem_size, 53);
		asserteq(vec.size, 0);
	}

	before_each() {
		vector_init(&vec, sizeof(int));
	}

	it("allocates vectors based on elem_size") {
		vector_alloc(&vec, 10);
		asserteq(vec.elem_size, sizeof(int));
		asserteq(vec.count, 0);
		assert(vec.size >= 10);
		/* Not an assert, but will cause valgrind to complain
		 * if not enough memory was allocated */
		memset(vec.elems, 0xff, 10 * vec.elem_size);
	}

	subdesc(vector_set) {
		it("sets values inside of the allocated range") {
			vector_alloc(&vec, 2);
			int el = 10;
			vector_set(&vec, 1, &el);
			asserteq(*(int *)((char *)vec.elems + sizeof(int)), 10);
		}

		it("allocates space when setting values outside the allocated range") {
			int el = 10;
			vector_set(&vec, 50, &el);
			asserteq(*(int *)((char *)vec.elems + (sizeof(int) * 50)), 10);
		}
	}

	subdesc(vector_get) {
		it("gets values inside the allocated range") {
			int el = 500;
			vector_set(&vec, 10, &el);
			asserteq(*(int *)vector_get(&vec, 10), 500);
		}

		it("returns NULL when trying to access values outside the allocated range") {
			int el = 10;
			vector_set(&vec, 100, &el);
			asserteq(vector_get(&vec, 101), NULL);
		}
	}
}
