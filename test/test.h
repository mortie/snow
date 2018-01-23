#ifndef TEST_H
#define TEST_H

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

#define fail(...) \
	do { \
		_test_exit_code = 1; \
		fprintf(stderr, \
			_TEST_COLOR_BOLD _TEST_COLOR_FAIL "%s✕ " \
			_TEST_COLOR_RESET _TEST_COLOR_FAIL "Failed:  " \
			_TEST_COLOR_RESET _TEST_COLOR_DESC "%s:\n%s    " \
			_TEST_COLOR_RESET, \
			_test_spaces, _test_desc, _test_spaces); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, \
			"\n%s    in %s:%s\n", _test_spaces, __FILE__, _test_name); \
		goto _test_done; \
	} while (0)

#define assert(x) \
	do { \
		if (!(x)) { \
			fail("Assertion failed: " #x); \
		} \
	} while (0)

#define asserteq(a, b) \
	do { \
		ssize_t _a = (ssize_t)(a); \
		ssize_t _b = (ssize_t)(b); \
		if (_a != _b) { \
			fail("Expected " #a " to equal " #b ", but got %zi", _a); \
		} \
	} while (0)

#define assertneq(a, b) \
	do { \
		ssize_t _a = (ssize_t)(a); \
		ssize_t _b = (ssize_t)(b); \
		if (_a == _b) { \
			fail("Expected " #a " to not equal " #b); \
		} \
	} while (0)

#define assertstreq(a, b) \
	do { \
		char *_a = (char *)(a); \
		char *_b = (char *)(b); \
		if (strcmp(_a, _b) != 0) { \
			fail("Expected " #a " to equal " #b ", but got %s", _a); \
		} \
	} while (0)

#define assertstrneq(a, b) \
	do { \
		char *_a = (char *)(a); \
		char *_b = (char *)(b); \
		if (strcmp(_a, _b) == 0) { \
			fail("Expected " #a " to not equal " #b); \
		} \
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
			_test_labelcnt -= 1; \
			if (_test_labelcnt >= 0) \
				goto *_test_labels[_test_labelcnt]; \
			else \
				goto _test_done; \
		} else { \
			_test_labels[_test_labelcnt++] = &&_test_defer_label; \
		} \
	} while (0)

#define it(testdesc, block) \
	do { \
		__label__ _test_done; \
		void *_test_labels[256]; \
		int _test_labelcnt = 0; \
		int __attribute__((unused)) _test_rundefer = 0; \
		char *_test_desc = testdesc; \
		_test_total += 1; \
		block \
		_test_successes += 1; \
		_test_print_success(); \
		_test_done: \
		__attribute__((unused)); \
		_test_rundefer = 1; \
		_test_labelcnt -= 1; \
		if (_test_labelcnt >= 0) { \
			goto *_test_labels[_test_labelcnt]; \
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
		char _test_spaces[_test_depth * 2 + 1]; \
		for (int i = 0; i < _test_depth * 2; ++i) \
			_test_spaces[i] = ' '; \
		_test_spaces[_test_depth * 2] = '\0'; \
		_test_print_run(); \
		block \
		_test_print_done(); \
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
		if (_test_num_defines > 1) { \
			fprintf(stderr, \
				_TEST_COLOR_BOLD "Total: Passed %i/%i tests.\n\n" \
				_TEST_COLOR_RESET, \
				_test_global_successes, _test_global_total); \
		} \
		return _test_exit_code; \
	} while (0)

#endif
