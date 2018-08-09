#ifdef SNOW_ENABLED

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <setjmp.h>

/*
 * Array
 */

struct _snow_arr {
	size_t elem_size;
	unsigned char *elems;
	size_t length;
	size_t allocated;
};

static void _snow_arr_init(struct _snow_arr *arr, size_t size) {
	arr->elem_size = size;
	arr->elems = NULL;
	arr->length = 0;
	arr->allocated = 0;
}

static void *_snow_arr_get(struct _snow_arr *arr, size_t index) {
	return arr->elems + arr->elem_size * index;
}

static void _snow_arr_push(struct _snow_arr *arr, void *elem) {
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

static void *_snow_arr_pop(struct _snow_arr *arr) {
	void *elem = _snow_arr_get(arr, arr->length - 1);
	arr->length -= 1;
	return elem;
}

static void *_snow_arr_top(struct _snow_arr *arr) {
	return _snow_arr_get(arr, arr->length - 1);
}

static void _snow_arr_reset(struct _snow_arr *arr) {
	free(arr->elems);
	arr->elems = NULL;
	arr->length = 0;
	arr->allocated = 0;
}

/*
 * Snow Core
 */

struct _snow_desc {
	const char *name;
	double start_time;
	int num_tests;
	int num_success;
};

struct _snow_desc_func {
	char *name;
	void (*func)();
};

struct _snow {
	int exit_code;
	struct _snow_arr desc_funcs;
	struct _snow_arr desc_stack;
	struct _snow_desc *current_desc;

	int in_case;
	struct {
		const char *name;
		double start_time;
		struct _snow_arr defers;
		jmp_buf jmp;
	} current_case;
};

extern struct _snow _snow;
extern int _snow_inited;

static void _snow_init() {
	_snow_inited = 1;
	_snow.exit_code = EXIT_SUCCESS;
	_snow_arr_init(&_snow.desc_funcs, sizeof(struct _snow_desc_func));
	_snow_arr_init(&_snow.desc_stack, sizeof(struct _snow_desc));
	_snow_arr_init(&_snow.current_case.defers, sizeof(jmp_buf));
	_snow.current_desc = NULL;
	_snow.in_case = 0;
}

static double _snow_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

__attribute__((unused))
static void _snow_desc_begin(const char *name) {
	struct _snow_desc desc = { name, _snow_now(), 0, 0 };
	_snow_arr_push(&_snow.desc_stack, &desc);
	printf("pushing %s\n", desc.name);

	_snow.current_desc =
		(struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);
}

__attribute__((unused))
static void _snow_desc_end() {
	struct _snow_desc *desc =
		(struct _snow_desc *)_snow_arr_pop(&_snow.desc_stack);
	printf("popping %s\n", desc->name);

	if (_snow.desc_stack.length > 0) {
		_snow.current_desc =
			(struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);
	} else {
		_snow.current_desc = NULL;
	}
}

__attribute__((unused))
static void _snow_case_begin(const char *name) {
	_snow.in_case = 1;
	_snow.current_case.name = name;
	_snow.current_case.start_time = _snow_now();
	_snow_arr_reset(&_snow.current_case.defers);

	int ret = setjmp(_snow.current_case.jmp);
	if (ret == 1) {
		if (_snow.current_case.defers.length > 0) {
			jmp_buf *jmp = (jmp_buf *)_snow_arr_pop(&_snow.current_case.defers);
			longjmp(*jmp, 1);
		}
	}
}

__attribute__((unused))
static void _snow_case_end(int success) {
	if (!_snow.in_case)
		return;

	_snow.in_case = 0;
	if (success) {
		_snow.current_desc->num_success += 1;
	}

	longjmp(_snow.current_case.jmp, 1);
}

__attribute__((unused))
static void _snow_case_defer_push(jmp_buf jmp) {
	_snow_arr_push(&_snow.current_case.defers, jmp);
}

__attribute__((unused))
static void _snow_case_defer_jmp() {
	longjmp(_snow.current_case.jmp, 1);
}

__attribute__((unused))
static int _snow_main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	for (size_t i = 0; i < _snow.desc_funcs.length; ++i) {
		struct _snow_desc_func *df = _snow_arr_get(&_snow.desc_funcs, i);
		_snow_desc_begin(df->name);
		df->func();
		_snow_desc_end();
	}

	return _snow.exit_code;
}

#define _snow_after_block(expr) \
	for (int _snow_done = 0; _snow_done == 0; ((expr), _snow_done += 1))

/*
 * Interface
 */

#define describe(name) \
	static void snow_test_##name(); \
	__attribute__((constructor (__COUNTER__ + 101))) \
	static void _snow_constructor_##name() { \
		if (!_snow_inited) _snow_init(); \
		struct _snow_desc_func df = { #name, &snow_test_##name }; \
		_snow_arr_push(&_snow.desc_funcs, &df); \
	} \
	static void snow_test_##name()

#define subdesc(name) \
	_snow_desc_begin(#name); \
	_snow_after_block(_snow_desc_end())

#define it(name) \
	_snow_case_begin(name); \
	_snow_after_block(_snow_case_end(1))
#define test it

#define defer(...) \
	do { \
		jmp_buf jmp; \
		if (setjmp(jmp) == 0) { \
			_snow_case_defer_push(jmp); \
		} else { \
			__VA_ARGS__; \
			_snow_case_defer_jmp(); \
		} \
	} while (0)

#define snow_main() \
	struct _snow _snow; \
	int _snow_inited = 0;
	int main(int argc, char **argv) { \
		return _snow_main(argc, argv); \
	}

#endif
