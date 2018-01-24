#ifndef SNOW_H
#define SNOW_H

#ifndef __GNUC__
#error "Your compiler doesn't support GNU extensions."
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef SNOW_COLOR_SUCCESS
#define SNOW_COLOR_SUCCESS "\033[32m"
#endif

#ifndef SNOW_COLOR_FAIL
#define SNOW_COLOR_FAIL "\033[31m"
#endif

#ifndef SNOW_COLOR_DESC
#define SNOW_COLOR_DESC "\033[1m\033[33m"
#endif

#define _SNOW_COLOR_BOLD    "\033[1m"
#define _SNOW_COLOR_RESET   "\033[0m"

static int _snow_exit_code = 0;
static int _snow_first_define = 1;
static int _snow_global_total = 0;
static int _snow_global_successes = 0;
static int _snow_num_defines = 0;

static int _snow_opt_color = 1;

struct {
	void **labels;
	size_t size;
	size_t count;
} _snow_labels = { NULL, 0, 0 };

#define _snow_fail(desc, spaces, name, file, ...) \
	do { \
		_snow_exit_code = 1; \
		if (_snow_opt_color) { \
			fprintf(stdout, \
				_SNOW_COLOR_BOLD SNOW_COLOR_FAIL "%s✕ " \
				_SNOW_COLOR_RESET SNOW_COLOR_FAIL "Failed:  " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
				_SNOW_COLOR_RESET ":\n%s    ", \
				spaces, desc, spaces); \
		} else { \
			fprintf(stdout, \
				"%s✕ Failed:  %s:\n%s    ", \
				spaces, desc, spaces); \
		} \
		fprintf(stdout, __VA_ARGS__); \
		fprintf(stdout, \
			"\n%s    in %s:%s\n", spaces, file, name); \
	} while (0)

static int __attribute__((unused)) _snow_asserteq_int(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		intmax_t a, intmax_t b)
{
	if (a != b)
	{
		_snow_fail(
			desc, spaces, name, file,
			"Expected %s to equal %s, but got %ji",
			astr, bstr, a);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _snow_assertneq_int(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		intmax_t a, intmax_t b)
{
	if (a == b)
	{
		_snow_fail(
			desc, spaces, name, file,
			"Expected %s to not equal %s",
			astr, bstr);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _snow_asserteq_str(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		const char *a, const char *b)
{
	if (strcmp(a, b) != 0)
	{
		_snow_fail(
			desc, spaces, name, file,
			"Expected %s to equal %s, but got \"%s\"",
			astr, bstr, a);
		return -1;
	}
	return 0;
}

static int __attribute__((unused)) _snow_assertneq_str(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr,
		const char *a, const char *b)
{
	if (strcmp(a, b) == 0)
	{
		_snow_fail(
			desc, spaces, name, file,
			"Expected %s to not equal %s",
			astr, bstr);
		return -1;
	}
	return 0;
}

#define fail(...) \
	do { \
		_snow_fail(_snow_desc, _snow_spaces, _snow_name, __FILE__, __VA_ARGS__); \
		goto _snow_done; \
	} while (0)

#define assert(x) \
	do { \
		if (!(x)) { \
			fail("Assertion failed: " #x); \
		} \
	} while (0)

#define asserteq_int(a, b) \
	do { \
		if (_snow_asserteq_int(_snow_desc, _snow_spaces, _snow_name, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

#define assertneq_int(a, b) \
	do { \
		if (_snow_assertneq_int(_snow_desc, _snow_spaces, _snow_name, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

#define asserteq_str(a, b) \
	do { \
		if (_snow_asserteq_str(_snow_desc, _snow_spaces, _snow_name, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

#define assertneq_str(a, b) \
	do { \
		if (_snow_assertneq_str(_snow_desc, _snow_spaces, _snow_name, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

#define asserteq(a, b) \
	do { \
		int r = _Generic((b), \
			char *: _snow_asserteq_str( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			const char *: _snow_asserteq_str( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			default: _snow_asserteq_int( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (intmax_t)a, (intmax_t)b) \
		); \
		if (r < 0) \
			goto _snow_done; \
	} while (0)

#define assertneq(a, b) \
	do { \
		int r = _Generic((b), \
			char *: _snow_assertneq_str( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			const char *: _snow_assertneq_str( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (const char *)a, (const char *)b), \
			default: _snow_assertneq_int( \
				_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (intmax_t)a, (intmax_t)b) \
		); \
		if (r < 0) \
			goto _snow_done; \
	} while (0)

#define _snow_print_success() \
	do { \
		if (_snow_opt_color) { \
			fprintf(stdout, \
				_SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "%s✓ " \
				_SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
				_SNOW_COLOR_RESET "\n", \
				_snow_spaces, _snow_desc); \
		} else { \
			fprintf(stdout, \
				"%s✓ Success: %s\n", \
				_snow_spaces, _snow_desc); \
		} \
	} while (0)

#define _snow_print_run() \
	do { \
		if (_snow_depth > 0 || _snow_first_define) { \
			fprintf(stdout, "\n"); \
			_snow_first_define = 0; \
		} \
		if (_snow_opt_color) { \
			fprintf(stdout, \
				_SNOW_COLOR_BOLD "%sTesting %s:" _SNOW_COLOR_RESET "\n", \
				_snow_spaces, _snow_name); \
		} else { \
			fprintf(stdout, \
				"%sTesting %s:\n", \
				_snow_spaces, _snow_name); \
		} \
	} while (0)

#define _snow_print_done() \
	do { \
		if (_snow_opt_color) { \
			fprintf(stdout, \
				_SNOW_COLOR_BOLD "%s%s: Passed %i/%i tests." \
				_SNOW_COLOR_RESET "\n\n", \
				_snow_spaces, _snow_name, _snow_successes, _snow_total); \
		} else { \
			fprintf(stdout, \
				"%s%s: Passed %i/%i tests.\n\n", \
				_snow_spaces, _snow_name, _snow_successes, _snow_total); \
		} \
	} while (0)

#define defer(expr) \
	do { \
		__label__ _snow_defer_label; \
		_snow_defer_label: \
		if (_snow_rundefer) { \
			expr; \
			/* Go to the previous defer, or the end of the `it` block */ \
			if (_snow_labels.count > 0) \
				goto *_snow_labels.labels[--_snow_labels.count]; \
			else \
				goto _snow_done; \
		} else { \
			_snow_labels.count += 1; \
			/* Realloc labels array if necessary */ \
			if (_snow_labels.count >= _snow_labels.size) { \
				if (_snow_labels.size == 0) \
					_snow_labels.size = 3; \
				else \
					_snow_labels.size *= 2; \
				_snow_labels.labels = realloc( \
					_snow_labels.labels, \
					_snow_labels.size * sizeof(*_snow_labels.labels)); \
			} \
			/* Add pointer to label to labels array */ \
			_snow_labels.labels[_snow_labels.count - 1] = \
				&&_snow_defer_label; \
		} \
	} while (0)

#define it(testdesc, block) \
	do { \
		__label__ _snow_done; \
		int __attribute__((unused)) _snow_rundefer = 0; \
		char *_snow_desc = testdesc; \
		_snow_total += 1; \
		block \
		_snow_successes += 1; \
		_snow_print_success(); \
		_snow_done: \
		__attribute__((unused)); \
		_snow_rundefer = 1; \
		if (_snow_labels.count > 0) { \
			_snow_labels.count -= 1; \
			goto *_snow_labels.labels[_snow_labels.count]; \
		} \
	} while (0)

#define subdesc(testname, block) \
	do { \
		int *_snow_parent_total = &_snow_total; \
		int *_snow_parent_successes = &_snow_successes; \
		int _snow_parent_depth = _snow_depth; \
		char *_snow_name = #testname; \
		int __attribute__((unused)) _snow_depth = _snow_parent_depth + 1; \
		int _snow_successes = 0; \
		int _snow_total = 0; \
		/* Malloc because Clang doesn't like using a variable length
		 * stack allocated array here, because dynamic gotos */ \
		char *_snow_spaces = malloc(_snow_depth * 2 + 1); \
		for (int i = 0; i < _snow_depth * 2; ++i) \
			_snow_spaces[i] = ' '; \
		_snow_spaces[_snow_depth * 2] = '\0'; \
		_snow_print_run(); \
		block \
		_snow_print_done(); \
		free(_snow_spaces); \
		*_snow_parent_successes += _snow_successes; \
		*_snow_parent_total += _snow_total; \
	} while(0)

#define describe(testname, block) \
	static void test_##testname() { \
		_snow_num_defines += 1; \
		char *_snow_name = #testname; \
		int __attribute__((unused)) _snow_depth = 0; \
		int _snow_successes = 0; \
		int _snow_total = 0; \
		const char *_snow_spaces = ""; \
		_snow_print_run(); \
		block \
		_snow_print_done(); \
		_snow_global_successes += _snow_successes; \
		_snow_global_total += _snow_total; \
	}

#define snow_main(block) \
	int main(int argc, char **argv) { \
		if (!isatty(1)) \
			_snow_opt_color = 0; \
		else if (getenv("NO_COLOR") != NULL) \
			_snow_opt_color = 0; \
		for (int i = 1; i < argc; ++i) { \
			if (strcmp(argv[i], "--color") == 0) \
				_snow_opt_color = 1; \
			else if (strcmp(argv[i], "--no-color") == 0) \
				_snow_opt_color = 0; \
		} \
		block \
		free(_snow_labels.labels); \
		if (_snow_num_defines > 1) { \
			if (_snow_opt_color) { \
				fprintf(stdout, \
					_SNOW_COLOR_BOLD "Total: Passed %i/%i tests.\n\n" \
					_SNOW_COLOR_RESET, \
					_snow_global_successes, _snow_global_total); \
			} else { \
				fprintf(stdout, \
					"Total: Passed %i/%i tests.\n\n", \
					_snow_global_successes, _snow_global_total); \
			} \
		} \
		return _snow_exit_code; \
	}

#endif
