#ifndef TEST_H
#define TEST_H

#ifndef __GNUC__
#error "Your compiler doesn't support GNU extensions."
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define _TEST_COLOR_SUCCESS "\033[32m"
#define _TEST_COLOR_FAIL    "\033[31m"
#define _TEST_COLOR_DESC    "\033[1m\033[33m"
#define _TEST_COLOR_BOLD    "\033[1m"
#define _TEST_COLOR_RESET   "\033[0m"

static int _test_exit_code = 0;
static int _test_first_define = 1;
static int _test_global_total = 0;
static int _test_global_successes = 0;
static int _test_num_defines = 0;

struct {
	void **labels;
	size_t size;
	size_t count;
} _test_labels = { NULL, 0, 0 };

#define _test_fail(desc, spaces, name, file, ...) \
	do { \
		_test_exit_code = 1; \
		fprintf(stderr, \
			_TEST_COLOR_BOLD _TEST_COLOR_FAIL "%s✕ " \
			_TEST_COLOR_RESET _TEST_COLOR_FAIL "Failed:  " \
			_TEST_COLOR_RESET _TEST_COLOR_DESC "%s" \
			_TEST_COLOR_RESET ":\n%s    ", \
			spaces, desc, spaces); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, \
			"\n%s    in %s:%s\n", spaces, file, name); \
	} while (0)

static int __attribute__((unused)) _test_asserteq_int(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		intmax_t a, intmax_t b)
{
	if (a != b)
	{
		_test_fail(
			desc, spaces, name, file,
			"Expected %s to equal %s, but got %ji",
			astr, bstr, a);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _test_assertneq_int(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		intmax_t a, intmax_t b)
{
	if (a == b)
	{
		_test_fail(
			desc, spaces, name, file,
			"Expected %s to not equal %s",
			astr, bstr);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _test_asserteq_str(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		const char *a, const char *b)
{
	if (strcmp(a, b) != 0)
	{
		_test_fail(
			desc, spaces, name, file,
			"Expected %s to equal %s, but got \"%s\"",
			astr, bstr, a);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _test_assertneq_str(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		const char *a, const char *b)
{
	if (strcmp(a, b) == 0)
	{
		_test_fail(
			desc, spaces, name, file,
			"Expected %s to not equal %s",
			astr, bstr);
		return -1;
	}
	return 0;
}

#define fail(...) \
	do { \
		_test_fail(_test_desc, _test_spaces, _test_name, __FILE__, __VA_ARGS__); \
		goto _test_done; \
	} while (0)

#define assert(x) \
	do { \
		if (!(x)) { \
			fail("Assertion failed: " #x); \
		} \
	} while (0)

#define asserteq_int(a, b) \
	do { \
		if (_test_asserteq_int(_test_desc, _test_spaces, _test_name, #a, #b, (a), (b)) < 0) \
			goto _test_done; \
	} while (0)

#define assertneq_int(a, b) \
	do { \
		if (_test_assertneq_int(_test_desc, _test_spaces, _test_name, #a, #b, (a), (b)) < 0) \
			goto _test_done; \
	} while (0)

#define asserteq_str(a, b) \
	do { \
		if (_test_asserteq_str(_test_desc, _test_spaces, _test_name, #a, #b, (a), (b)) < 0) \
			goto _test_done; \
	} while (0)

#define assertneq_str(a, b) \
	do { \
		if (_test_assertneq_str(_test_desc, _test_spaces, _test_name, #a, #b, (a), (b)) < 0) \
			goto _test_done; \
	} while (0)

#define asserteq(a, b) \
	do { \
		int r = _Generic((b), \
			char *: _test_asserteq_str( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			const char *: _test_asserteq_str( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			default: _test_asserteq_int( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (intmax_t)a, (intmax_t)b) \
		); \
		if (r < 0) \
			goto _test_done; \
	} while (0)

#define assertneq(a, b) \
	do { \
		int r = _Generic((b), \
			char *: _test_assertneq_str( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			const char *: _test_assertneq_str( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			default: _test_assertneq_int( \
				_test_desc, _test_spaces, _test_name, __FILE__, #a, #b, (intmax_t)a, (intmax_t)b) \
		); \
		if (r < 0) \
			goto _test_done; \
	} while (0)

#define _test_print_success() \
	do { \
		fprintf(stderr, \
			_TEST_COLOR_BOLD _TEST_COLOR_SUCCESS "%s✓ " \
			_TEST_COLOR_RESET _TEST_COLOR_SUCCESS "Success: " \
			_TEST_COLOR_RESET _TEST_COLOR_DESC "%s" \
			_TEST_COLOR_RESET "\n", \
			_test_spaces, _test_desc); \
	} while (0)

#define _test_print_run() \
	do { \
		if (_test_depth > 0 || _test_first_define) { \
			fprintf(stderr, "\n"); \
			_test_first_define = 0; \
		} \
		fprintf(stderr, _TEST_COLOR_BOLD "%sTesting %s:" _TEST_COLOR_RESET "\n", \
			_test_spaces, _test_name); \
	} while (0)

#define _test_print_done() \
	do { \
		fprintf(stderr, \
			_TEST_COLOR_BOLD "%s%s: Passed %i/%i tests." \
			_TEST_COLOR_RESET "\n\n", \
			_test_spaces, _test_name, _test_successes, _test_total); \
	} while (0)

#define defer(expr) \
	do { \
		__label__ _test_defer_label; \
		_test_defer_label: \
		if (_test_rundefer) { \
			expr; \
			/* Go to the previous defer, or the end of the `it` block */ \
			if (_test_labels.count > 0) \
				goto *_test_labels.labels[--_test_labels.count]; \
			else \
				goto _test_done; \
		} else { \
			_test_labels.count += 1; \
			/* Realloc labels array if necessary */ \
			if (_test_labels.count >= _test_labels.size) { \
				if (_test_labels.size == 0) \
					_test_labels.size = 3; \
				else \
					_test_labels.size *= 2; \
				_test_labels.labels = realloc( \
					_test_labels.labels, \
					_test_labels.size * sizeof(*_test_labels.labels)); \
			} \
			/* Add pointer to label to labels array */ \
			_test_labels.labels[_test_labels.count - 1] = \
				&&_test_defer_label; \
		} \
	} while (0)

#define it(testdesc, block) \
	do { \
		__label__ _test_done; \
		int __attribute__((unused)) _test_rundefer = 0; \
		char *_test_desc = testdesc; \
		_test_total += 1; \
		block \
		_test_successes += 1; \
		_test_print_success(); \
		_test_done: \
		__attribute__((unused)); \
		_test_rundefer = 1; \
		if (_test_labels.count > 0) { \
			_test_labels.count -= 1; \
			goto *_test_labels.labels[_test_labels.count]; \
		} \
	} while (0)

#define subdesc(testname, block) \
	do { \
		int *_test_parent_total = &_test_total; \
		int *_test_parent_successes = &_test_successes; \
		int _test_parent_depth = _test_depth; \
		char *_test_name = #testname; \
		int __attribute__((unused)) _test_depth = _test_parent_depth + 1; \
		int _test_successes = 0; \
		int _test_total = 0; \
		/* Malloc because Clang doesn't like using a variable length
		 * stack allocated array here, because dynamic gotos */ \
		char *_test_spaces = malloc(_test_depth * 2 + 1); \
		for (int i = 0; i < _test_depth * 2; ++i) \
			_test_spaces[i] = ' '; \
		_test_spaces[_test_depth * 2] = '\0'; \
		_test_print_run(); \
		block \
		_test_print_done(); \
		free(_test_spaces); \
		*_test_parent_successes += _test_successes; \
		*_test_parent_total += _test_total; \
	} while(0)

#define describe(testname, block) \
	static void test_##testname() { \
		_test_num_defines += 1; \
		char *_test_name = #testname; \
		int __attribute__((unused)) _test_depth = 0; \
		int _test_successes = 0; \
		int _test_total = 0; \
		const char *_test_spaces = ""; \
		_test_print_run(); \
		block \
		_test_print_done(); \
		_test_global_successes += _test_successes; \
		_test_global_total += _test_total; \
	}

#define done() \
	do { \
		free(_test_labels.labels); \
		if (_test_num_defines > 1) { \
			fprintf(stderr, \
				_TEST_COLOR_BOLD "Total: Passed %i/%i tests.\n\n" \
				_TEST_COLOR_RESET, \
				_test_global_successes, _test_global_total); \
		} \
		return _test_exit_code; \
	} while (0)

#endif
