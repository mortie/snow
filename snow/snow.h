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
#include <inttypes.h>
#include <setjmp.h>

#ifndef SNOW_COLOR_SUCCESS
#define SNOW_COLOR_SUCCESS "\033[32m"
#endif

#ifndef SNOW_COLOR_MAYBE
#define SNOW_COLOR_MAYBE "\033[35m"
#endif

#ifndef SNOW_COLOR_FAIL
#define SNOW_COLOR_FAIL "\033[31m"
#endif

#ifndef SNOW_COLOR_DESC
#define SNOW_COLOR_DESC "\033[1m\033[33m"
#endif

#define _SNOW_COLOR_BOLD "\033[1m"
#define _SNOW_COLOR_RESET "\033[0m"

#define SNOW_MAX_DEPTH 128

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

/*
 * Spaces
 */

static char _snow_spaces_str[SNOW_MAX_DEPTH * 2 + 1];
static int _snow_spaces_depth_prev = 0;
char *_snow_spaces(int depth) {
	if (depth > SNOW_MAX_DEPTH)
		depth = SNOW_MAX_DEPTH;

	_snow_spaces_str[depth * 2] = '\0';
	if (depth > _snow_spaces_depth_prev)
		memset(_snow_spaces_str, ' ', depth * 2);
	_snow_spaces_depth_prev = depth;

	return _snow_spaces_str;
}

/*
 * Print stuff
 */

#define _snow_print(...) \
	printf(__VA_ARGS__)

#define _snow_printd(context, offs, ...) \
	do { \
		_snow_print("\r%s", _snow_spaces((context)->depth + (offs))); \
		_snow_print(__VA_ARGS__); \
	} while (0)

#define _SNOW_NL_DESC 1
#define _SNOW_NL_CASE 2
#define _SNOW_NL_RES 3
static int _snow_nl = 0;

#define _snow_print_testing(context, name) \
	_snow_print("\n"); \
	_snow_nl = _SNOW_NL_DESC; \
	_snow_printd(context, 0, \
		_SNOW_COLOR_BOLD "Testing %s" _SNOW_COLOR_RESET ":\n", \
		name)

#define _snow_print_result(context, name, passed, total) \
	if (_snow_nl != _SNOW_NL_CASE) \
		_snow_print("\n"); \
	_snow_nl = _SNOW_NL_RES; \
	_snow_printd(context, -1, \
		_SNOW_COLOR_BOLD "%s: Passed %i/%i tests.\n" \
		_SNOW_COLOR_RESET, \
		name, passed, total); \

#define _snow_print_maybe(context, name) \
	do { \
		_snow_printd(context, -1, \
			_SNOW_COLOR_BOLD SNOW_COLOR_MAYBE "? " \
			_SNOW_COLOR_RESET SNOW_COLOR_MAYBE "Testing: " \
			_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s: " _SNOW_COLOR_RESET, \
			name); \
		fflush(stdout); \
	} while (0)

#define _snow_print_success(context, name) \
	_snow_nl = _SNOW_NL_CASE; \
	_snow_printd(context, -1, \
		_SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "✓ " \
		_SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: " \
		_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s\n" \
		_SNOW_COLOR_RESET, \
		name)

#define _snow_print_failure(context, name) \
	_snow_nl = _SNOW_NL_CASE; \
	_snow_printd(context, -1, \
		_SNOW_COLOR_BOLD SNOW_COLOR_FAIL "✕ " \
		_SNOW_COLOR_RESET SNOW_COLOR_FAIL "Failed:  " \
		_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
		_SNOW_COLOR_RESET ":\n", \
		name)

struct _snow_context {
	char *name;
	int depth;
	struct _snow_context *parent;
	int num_tests;
	int num_success;
	int done;
	int in_case;
};

static jmp_buf _snow_case_start;

struct _snow_case_context {
	char *name;
	char *filename;
	int linenum;
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
#define describe(descname, ...) \
	static void snow_test_##descname(struct _snow_context *_snow_context); \
	__attribute__((constructor (__COUNTER__ + 101))) \
	static void _snow_constructor_##descname() { \
		struct _snow_desc desc = { \
			.func = &snow_test_##descname, \
			.name = #descname, \
		}; \
		_snow_arr_append(&_snow_descs, &desc); \
	} \
	static void snow_test_##descname( \
		struct _snow_context *_snow_context) __VA_ARGS__

/*
 * This is a beautiful hack to use a for loop to run code _after_ the block
 * which appears after the macro.
 */
int _snow_subdesc_after(struct _snow_context *context) {
	context->done = 1;
	context->parent->num_tests += context->num_tests;
	context->parent->num_success += context->num_success;
	_snow_print_result(context, context->name, 
		context->num_tests, context->num_success);
	return 0;
}
#define subdesc(descname, ...) \
	_snow_print_testing(_snow_context, #descname); \
	for ( \
		struct _snow_context _snow_ctx = { \
				.name = #descname, \
				.depth = _snow_context->depth + 1, \
				.parent = _snow_context, \
				.num_tests = 0, \
				.num_success = 0, \
				.done = 0, \
			}, \
			*_snow_context = &_snow_ctx; \
		_snow_context->done == 0; \
		_snow_subdesc_after(_snow_context)) __VA_ARGS__

#define it(casename, ...) \
	_snow_context->num_tests += 1; \
	_snow_context->in_case = 1; \
	{ \
		int jmpret = setjmp(_snow_case_start); \
		if (jmpret != 0) { \
			_snow_context->in_case = 0; \
			if (jmpret == 1) { \
				_snow_context->num_success += 1; \
				_snow_print_success(_snow_context, casename); \
			} else if (jmpret == -1) { \
			} \
		} else { \
			_snow_print_maybe(_snow_context, casename); \
		} \
	} \
	for ( \
		struct _snow_case_context _snow_case_ctx = { casename, __FILE__, __LINE__ }, \
			__attribute__((unused)) *_snow_case_context = &_snow_case_ctx; \
		_snow_context->in_case; \
		longjmp(_snow_case_start, 1)) __VA_ARGS__
#define test it

/*
 * Main function.
 */
int _snow_main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	struct _snow_context root_context = { 0 };
	root_context.name = "Total";
	struct _snow_context context = { 0 };
	context.parent = &root_context;
	context.depth = 1;

	for (size_t i = 0; i < _snow_descs.length; i += 1) {
		struct _snow_desc *desc = _snow_arr_get(&_snow_descs, i);

		context.name = desc->name;

		_snow_print_testing(&root_context, context.name);
		desc->func(&context);
		_snow_print_result(&context,
			context.name, context.num_success, context.num_tests);
		root_context.num_tests += context.num_tests;
		root_context.num_success += context.num_success;
	}
	if (_snow_descs.length > 1) {
		_snow_print_result(&context, root_context.name,
			root_context.num_success, root_context.num_tests);
	}
	_snow_print("\n");
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

/*
 * Fail a test.
 */
#define fail(...) \
	do { \
		_snow_print_failure(_snow_context, _snow_case_context->name); \
		_snow_printd(_snow_context, -1, "    "); \
		_snow_print(__VA_ARGS__); \
		_snow_print("\n"); \
		_snow_printd(_snow_context, -1, "    in %s:%i\n", \
			_snow_case_context->filename, _snow_case_context->linenum); \
		longjmp(_snow_case_start, -1); \
	} while (0)

#define _snow_fail_expl(exp, fmt, ...) \
	do { \
		if ((exp) == NULL) \
			fail(fmt ".", __VA_ARGS__); \
		else \
			fail(fmt ": %s", __VA_ARGS__, (exp)); \
	} while (0)

/*
 * Assert that an expression is true
 */
#define assert(x, ...) \
	do { \
		char *expl = #__VA_ARGS__; \
		char *explanation = expl[0] == '\0' ? NULL : "" __VA_ARGS__; \
		if (!(x)) \
			_snow_fail_expl(explanation, "Assertion failed: %s", #x); \
	} while (0)

/*
 * Define assert functions
 */
#define _snow_define_assertfunc(name, type, pattern) \
	int _snow_assert_##name( \
			struct _snow_context *_snow_context, struct _snow_case_context *_snow_case_context, \
			int invert, char *explanation, \
			type a, char *astr, type b, char *bstr) { \
		int eq = (a) == (b); \
		if (!eq && !invert) \
			_snow_fail_expl(explanation, \
				"(" #name ") Expected %s to equal %s, but got " pattern, \
				astr, bstr, a); \
		else if (eq && invert) \
			_snow_fail_expl(explanation, \
				"(" #name ") Expected %s to not equal %s (" pattern ")", \
				astr, bstr, a); \
		return 0; \
	}
_snow_define_assertfunc(int, intmax_t, "%ji")
_snow_define_assertfunc(uint, uintmax_t, "%ju")
_snow_define_assertfunc(dbl, double, "%g")
_snow_define_assertfunc(ptr, void *, "%p")

int _snow_assert_str(
		struct _snow_context *_snow_context, struct _snow_case_context *_snow_case_context,
		int invert, char *explanation,
		char *a, char *astr, char *b, char *bstr) {
	int eq = strcmp(a, b) == 0;
	if (!eq && !invert)
		_snow_fail_expl(explanation,
			"(str) Expected %s to equal %s, but got \"%s\"",
			astr, bstr, a);
	else if (eq && invert)
		_snow_fail_expl(explanation,
			"(str) Expected %s to not equal %s (\"%s\")",
			astr, bstr, a);
	return 0;
}

int _snow_assert_mem(
		struct _snow_context *_snow_context, struct _snow_case_context *_snow_case_context,
		int invert, char *explanation,
		void *a, char *astr, size_t asize, void *b, char *bstr, size_t bsize)
{
	int cmp;
	if (asize == bsize)
		cmp = memcmp(a, b, asize);
	else
		cmp = asize > bsize ? -1 : 1;

	int eq = asize == bsize && cmp == 0;
	if (!eq && !invert) {
		if (cmp < 0)
			_snow_fail_expl(explanation, "(mem) Expected %s to equal %s (%s < %s)", \
				astr, bstr, astr, bstr);
		else
			_snow_fail_expl(explanation, "(mem) Expected %s to equal %s (%s > %s)",
				astr, bstr, astr, bstr);
	} else if (eq && invert) {
		_snow_fail_expl(explanation, "(mem) Expected %s to not equal %s",
			astr, bstr);
	}
	return 0;
}

int _snow_assert_fake() {
	return -1;
}

#define _snow_generic_assert(x) \
	_Generic((x), \
		float: _snow_assert_dbl, \
		double: _snow_assert_dbl, \
		void *: _snow_assert_ptr, \
		char *: _snow_assert_str, \
		int: _snow_assert_int, \
		long long: _snow_assert_int, \
		unsigned int: _snow_assert_uint, \
		unsigned long long: _snow_assert_uint, \
		size_t: _snow_assert_uint, \
		default: _snow_assert_fake)

#define asserteq(a, b, ...) \
	do { \
		char *expl = #__VA_ARGS__; \
		char *explanation = expl[0] == '\0' ? NULL : "" __VA_ARGS__; \
		if (sizeof(a) != sizeof(b)) { \
			_snow_fail_expl(explanation, \
				"Expected %s to equal %s, but their sizes don't match", \
				#a, #b); \
		} else { \
			int ret = _snow_generic_assert(b)( \
				_snow_context, _snow_case_context, 0, explanation, \
				(a), #a, (b), #b); \
			if (ret < 0) { \
				typeof (a) _a; \
				typeof (b) _b; \
				_snow_assert_mem( \
					_snow_context, _snow_case_context, 0, explanation, \
					&_a, #a, sizeof(_a), &_b, #b, sizeof(_b)); \
			} \
		} \
	} while (0)

#define assertneq(a, b, ...) \
	do { \
		char *expl = #__VA_ARGS__; \
		char *explanation = expl[0] == '\0' ? NULL : "" __VA_ARGS__; \
		if (sizeof(a) != sizeof(b)) { \
			break; \
		} else { \
			int ret = _snow_generic_assert(b)( \
				_snow_context, _snow_case_context, 1, explanation, \
				(a), #a, (b), #b); \
			if (ret < 0) { \
				typeof (a) _a; \
				typeof (b) _b; \
				_snow_assert_mem( \
					_snow_context, _snow_case_context, 1, explanation, \
					&_a, #a, sizeof(_a), &_b, #b, sizeof(_b)); \
			} \
		} \
	} while (0)

#endif
