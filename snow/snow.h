#ifndef SNOW_ENABLED

#define describe(testname) \
	__attribute__((unused)) void snow_unused_##testname()
#define subdesc(...)
#define it(...)
#define fail(...)
#define test(...)

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * General-purpose vector abstraction
 */
struct _snow_arr {
	size_t elem_size;
	unsigned char *elems;
	size_t length;
	size_t allocated;
};
static void _snow_arr_append(struct _snow_arr *arr, void *elem) {
	if (arr->allocated == 0) {
		arr->allocated = 8;
		arr->elems = malloc(arr->allocated * arr->elem_size);
	} else if (arr->allocated <= arr->length) {
		arr->allocated *= 2;
		arr->elems = realloc(arr->elems, arr->allocated * arr->elem_size);
	}
	memcpy(
		arr->elems + arr->length * arr->elem_size, elem, arr->elem_size);
	arr->length += 1;
}
static void *_snow_arr_get(struct _snow_arr *arr, size_t index) {
	return arr->elems + arr->elem_size * index;
}

struct _snow_context {
	char *name;
	int depth;
	struct _snow_context *parent;
	int num_tests;
	int num_success;
	int done;
};

struct _snow_case_context {
	char *name;
	int done;
};

struct _snow_desc {
	char *name;
	void (*func)(struct _snow_context *_snow_context);
};
extern struct _snow_arr _snow_descs;

/*
 * Define a function called snow_test_##testname,
 * and create an __attribute__((constructor)) function
 * to add a function pointer to snow_test_##testname to a list.
 */
#define describe(testname) \
	static void snow_test_##testname(struct _snow_context *_snow_context); \
	__attribute__((constructor (__COUNTER__ + 101))) \
	static void _snow_constructor_##testname() { \
		struct _snow_desc desc = { \
			.func = &snow_test_##testname, \
			.name = #testname, \
		}; \
		_snow_arr_append(&_snow_descs, &desc); \
	} \
	static void snow_test_##testname(struct _snow_context *_snow_context)

/*
 * This is a beautiful hack to use a for loop to run code _after_ the block
 * which appears after the macro.
 *
 * First statement: declare a stack-allocated _snow_ctx,
 * with the old _snow_context as the parent context.
 * Then declare a _snow_context, which is a pointer to _snow_ctx
 * and will shadow the old _snow_context variable.
 *
 * Second statement: _snow_Context->done starts as 0, but will be set to 1
 * before the second for-loop iteration.
 *
 * Third statement: This will run after the block!
 * It runs the _snow_subdesc_after function, which sets _snow_context->done to 1,
 * and does whatever else we want to do after the subdesc has executed.
 */
int _snow_subdesc_after(struct _snow_context *context) {
	context->done = 1;
	context->parent->num_tests += context->num_tests;
	context->parent->num_success += context->num_success;
	printf("%i %s: %i/%i.\n",
		context->depth, context->name,
		context->num_success, context->num_tests);
	return 0;
}
#define subdesc(name) \
	printf("%i testing %s.\n", _snow_context->depth, #name); \
	for ( \
		struct _snow_context _snow_ctx = \
			{ #name, _snow_context->depth + 1, _snow_context, 0, 0, 0 }, \
			*_snow_context = &_snow_ctx; \
		_snow_context->done == 0; \
		_snow_subdesc_after(_snow_context))

/*
 * An actual test case.
 * Uses the same for-loop trick as subdesc,
 * but doesn't have to worry about nesting.
 */
int _snow_case_after(
		struct _snow_context *context,
		struct _snow_case_context *case_context) {
	case_context->done = 1;
	context->num_success += 1;
	printf("%i Success: %s\n", context->depth, case_context->name);
	return 0;
}
#define it(name) \
	_snow_context->num_tests += 1; \
	for ( \
		struct _snow_case_context _snow_case_context = { name, 0 }; \
		_snow_case_context.done == 0; \
		_snow_case_after(_snow_context, &_snow_case_context))
#define test it

/*
 * Fail a test.
 */
#define fail(...) \
	printf("%i Failed: %s: ", _snow_context->depth, _snow_case_context.name); \
	printf(__VA_ARGS__); \
	printf("\n"); \
	break

/*
 * Main function.
 */
int _snow_main(int argc, char **argv) {
	struct _snow_context root_context = { "Total", 0, NULL, 0, 0, 0 };
	for (size_t i = 0; i < _snow_descs.length; i += 1) {
		struct _snow_desc *desc = _snow_arr_get(&_snow_descs, i);
		struct _snow_context context = { desc->name, 1, &root_context, 0, 0, 0 };
		printf("%i testing %s\n", root_context.depth, context.name);
		desc->func(&context);
		printf("%i %s: %i/%i.\n",
			root_context.depth, context.name,
			context.num_success, context.num_tests);
		root_context.num_tests += context.num_tests;
		root_context.num_success += context.num_success;
	}
	printf("%i %s: %i/%i.\n",
		root_context.depth, root_context.name,
		root_context.num_success, root_context.num_tests);
	return 0;
}

/*
 * Initialize the variables which other translation units will use as extern,
 * then just run _snow_main.
 */
#define snow_main() \
	struct _snow_arr _snow_descs = { sizeof(struct _snow_desc), NULL, 0, 0 }; \
	int main(int argc, char **argv) { \
		return _snow_main(argc, argv); \
	}

#endif
