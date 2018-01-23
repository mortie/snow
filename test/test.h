#ifndef TEST_H
#define TEST_H

#define _TEST_COLOR_SUCCESS "\033[32m"
#define _TEST_COLOR_FAIL    "\033[31m"
#define _TEST_COLOR_DESC    "\033[1m\033[33m"
#define _TEST_COLOR_BOLD    "\033[1m"
#define _TEST_COLOR_RESET   "\033[0m"

static int test_exit_code = 0;

#define fail(...) \
	do { \
		test_exit_code = 1; \
		fprintf(stderr, \
			_TEST_COLOR_BOLD _TEST_COLOR_FAIL "  %s✕ " \
			_TEST_COLOR_RESET _TEST_COLOR_FAIL "Failed:  " \
			_TEST_COLOR_RESET _TEST_COLOR_DESC "%s:\n  %s    " \
			_TEST_COLOR_RESET, \
			_test_spaces, _test_desc, _test_spaces); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, \
			"\n  %s    in %s:%s\n", _test_spaces, __FILE__, _test_name); \
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
		if ((a) != (b)) { \
			fail("Expected " #a " to equal " #b ", but got %i", (b)); \
		} \
	} while (0)

#define assertneq(a, b) \
	do { \
		if ((a) == (b)) { \
			fail("Expected " #a " to not equal " #b ", but got %i", (b)); \
		} \
	} while (0)

#define _test_print_success() \
	do { \
		fprintf(stderr, \
			_TEST_COLOR_BOLD _TEST_COLOR_SUCCESS "  %s✓ " \
			_TEST_COLOR_RESET _TEST_COLOR_SUCCESS "Success: " \
			_TEST_COLOR_RESET _TEST_COLOR_DESC "%s" \
			_TEST_COLOR_RESET "\n", \
			_test_spaces, _test_desc); \
	} while (0)

#define _test_print_run() \
	do { \
		fprintf(stderr, _TEST_COLOR_BOLD "%sTesting %s:" _TEST_COLOR_RESET "\n", \
			_test_spaces, _test_name); \
	} while (0)

#define _test_print_done() \
	do { \
		fprintf(stderr, \
			_TEST_COLOR_BOLD "%s%s: Passed %i/%i tests." \
			_TEST_COLOR_RESET "\n", \
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
		int _test_rundefer = 0; \
		char *_test_desc = testdesc; \
		_test_total += 1; \
		block \
		_test_successes += 1; \
		_test_print_success(); \
		_test_done: \
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
		int _test_depth = _test_parent_depth + 1; \
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
		char *_test_name = #testname; \
		int _test_depth = 0; \
		int _test_successes = 0; \
		int _test_total = 0; \
		const char *_test_spaces = ""; \
		_test_print_run(); \
		block \
		_test_print_done(); \
	}

#endif
