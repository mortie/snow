#ifndef SNOW_H
#define SNOW_H

#ifndef SNOW_ENABLED

#define describe(...)

#else

#ifndef __GNUC__
#error "Your compiler doesn't support GNU extensions."
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define SNOW_VERSION "1.1.0"

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

#define _SNOW_COLOR_BOLD    "\033[1m"
#define _SNOW_COLOR_RESET   "\033[0m"

// Compatibility with MinGW
#ifdef __MINGW32__
#  include <io.h>
#  ifdef __WIN64
#    define _SNOW_PRIuSIZE PRIu64
#  else
#    define _SNOW_PRIuSIZE PRIu32
#  endif
// cmd.exe is identified as a tty but doesn't understand escape sequences,
// git bash understand escape sequences but isn't identified as a TTY
// by _isatty(_fileno(stdout))
#  define _SNOW_ISATTY(file) 0
#else
#  define _SNOW_PRIuSIZE "zu"
#  define _SNOW_ISATTY(file) isatty(fileno(file))
// For stdio.h to define fileno, _POSIX_C_SOURCE or similar has to be defined
// before stdio.h is included. I want Snow to work without any other compiler
// flags than -DSNOW_ENABLED.
int fileno(FILE *stream);
#endif

extern FILE *_snow_log_file;

extern int _snow_exit_code;
extern int _snow_extra_newline;
extern int _snow_global_total;
extern int _snow_global_successes;
extern int _snow_num_defines;

struct _snow_labels { 
	void **labels;
	size_t size;
	size_t count;
};
extern struct _snow_labels _snow_labels;

struct _snow_describes { 
	void (**describes)();
	size_t size;
	size_t count;
};
extern struct _snow_describes _snow_describes;

typedef enum
{
	_snow_opt_version,
	_snow_opt_help,
	_snow_opt_color,
	_snow_opt_quiet,
	_snow_opt_maybes,
	_snow_opt_cr,
	_snow_opt_timer,
} _snow_opt_names;

struct _snow_option
{
	char *fullname;
	char shortname;
	int value;
	int overridden;
};
extern struct _snow_option _snow_opts[];

extern struct timeval _snow_timer;

#define _snow_print(...) \
	do { \
		fprintf(_snow_log_file, __VA_ARGS__); \
		fflush(_snow_log_file); \
	} while (0)

#define _snow_print_timer() \
	do { \
		if (_snow_opts[_snow_opt_timer].value) \
		{ \
			struct timeval now; \
			gettimeofday(&now, NULL); \
			double before_ms = _snow_timer.tv_sec * 1000.0 + _snow_timer.tv_usec / 1000.0; \
			double now_ms = now.tv_sec * 1000.0 + now.tv_usec / 1000.0; \
			double ms = now_ms - before_ms; \
			if (ms < 1000) \
				_snow_print(": %.2fms\n", ms); \
			else \
				_snow_print(": %.2fs\n", ms / 1000); \
		} \
		else \
			_snow_print("\n"); \
	} \
	while (0)

#define _snow_print_result_newline() \
	do { \
		if (_snow_opts[_snow_opt_maybes].value) {\
			if (_snow_opts[_snow_opt_cr].value) \
				_snow_print("\r"); \
			else \
				_snow_print("\n"); \
		} \
	} while (0)

#define _snow_fail(desc, spaces, name, file, ...) \
	do { \
		_snow_print_result_newline(); \
		_snow_extra_newline = 1; \
		_snow_exit_code = EXIT_FAILURE; \
		if (_snow_opts[_snow_opt_color].value) { \
			_snow_print( \
				_SNOW_COLOR_BOLD SNOW_COLOR_FAIL "%s✕ " \
				_SNOW_COLOR_RESET SNOW_COLOR_FAIL "Failed:  " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
				_SNOW_COLOR_RESET ":", \
				spaces, desc); \
		} else { \
			_snow_print("%s✕ Failed:  %s:", spaces, desc); \
		} \
		_snow_print("\n"); \
		_snow_print("%s    ", spaces); \
		_snow_print(__VA_ARGS__); \
		_snow_print("\n%s    in %s:%s\n", spaces, file, name); \
	} while (0)

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

#define _snow_decl_asserteq(suffix, type, fmt) \
	static int __attribute__((unused)) _snow_asserteq_##suffix( \
			const char *desc, const char *spaces, const char *name, const char *file, \
			const char *astr, const char *bstr, type a, type b) \
	{ \
		if (a != b) { \
			_snow_fail( \
				desc, spaces, name, file, \
				"Expected %s to equal %s, but got " fmt, \
				astr, bstr, a); \
			return -1; \
		} \
		return 0; \
	}
#define _snow_decl_assertneq(suffix, type) \
	static int __attribute__((unused)) _snow_assertneq_##suffix( \
			const char *desc, const char *spaces, const char *name, const char *file, \
			const char *astr, const char *bstr, type a, type b) \
	{ \
		if (a == b) { \
			_snow_fail( \
				desc, spaces, name, file, \
				"Expected %s to not equal %s", \
				astr, bstr); \
			return -1; \
		} \
		return 0; \
	}

_snow_decl_asserteq(int, intmax_t, "%" PRIdMAX)
_snow_decl_assertneq(int, intmax_t)
#define asserteq_int(a, b) \
	do { \
		if (_snow_asserteq_int(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq_int(a, b) \
	do { \
		if (_snow_assertneq_int(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

_snow_decl_asserteq(ptr, void *, "%p")
_snow_decl_assertneq(ptr, void *)
#define asserteq_ptr(a, b) \
	do { \
		if (_snow_asserteq_ptr(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq_ptr(a, b) \
	do { \
		if (_snow_assertneq_ptr(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

_snow_decl_asserteq(dbl, double, "%f");
_snow_decl_assertneq(dbl, double);
#define asserteq_dbl(a, b) \
	do { \
		if (_snow_asserteq_dbl(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq_dbl(a, b) \
	do { \
		if (_snow_assertneq_dbl(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

static int __attribute__((unused)) _snow_asserteq_str(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr, const char *a, const char *b)
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
		const char *astr, const char *bstr, const char *a, const char *b)
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
#define asserteq_str(a, b) \
	do { \
		if (_snow_asserteq_str(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq_str(a, b) \
	do { \
		if (_snow_assertneq_str(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b)) < 0) \
			goto _snow_done; \
	} while (0)

static int __attribute__((unused)) _snow_asserteq_buf(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr, const void *a, const void *b, size_t n)
{
	const char *_a = (const char *)a;
	const char *_b = (const char *)b;
	size_t i;
	for (i = 0; i < n; ++i)
	{
		if (_a[i] != _b[i])
		{
			_snow_fail(
				desc, spaces, name, file,
				"Expected %s to equal %s, but they differ at byte %" _SNOW_PRIuSIZE,
				astr, bstr, i);
			return -1;
		}
	}
	return 0;
}
static int __attribute__((unused)) _snow_assertneq_buf(
		const char *desc, const char *spaces, const char *name, const char *file,
		const char *astr, const char *bstr, const void *a, const void *b, size_t n)
{
	const char *_a = (const char *)a;
	const char *_b = (const char *)b;
	size_t i;
	for (i = 0; i < n; ++i)
	{
		if (_a[i] != _b[i])
			return 0;
	}
	_snow_fail(
		desc, spaces, name, file,
		"Expected %s to not equal %s",
		astr, bstr);
	return -1;
}
#define asserteq_buf(a, b, n) \
	do { \
		if (_snow_asserteq_buf(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b), (n)) < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq_buf(a, b, n) \
	do { \
		if (_snow_assertneq_buf(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, (a), (b), (n)) < 0) \
			goto _snow_done; \
	} while (0)

#if(__STDC_VERSION__ >= 201112L)

#define asserteq(a, b) \
	do { \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
		_Pragma("GCC diagnostic ignored \"-Wint-conversion\"") \
		int r = _Generic((b), \
			char *: _snow_asserteq_str, \
			const char *: _snow_asserteq_str, \
			float: _snow_asserteq_dbl, \
			double: _snow_asserteq_dbl, \
			default: _Generic((b) - (b), \
				int: _snow_asserteq_int, \
				default: _Generic((b) - (b), \
					ptrdiff_t: _snow_asserteq_ptr, \
					default: _snow_asserteq_int)) \
		)(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, a, b); \
		_Pragma("GCC diagnostic pop") \
		if (r < 0) \
			goto _snow_done; \
	} while (0)
#define assertneq(a, b) \
	do { \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
		_Pragma("GCC diagnostic ignored \"-Wint-conversion\"") \
		int r = _Generic((b), \
			char *: _snow_assertneq_str, \
			const char *: _snow_assertneq_str, \
			float: _snow_assertneq_dbl, \
			double: _snow_assertneq_dbl, \
			default: _Generic((b) - (b), \
				int: _snow_assertneq_int, \
				default: _Generic((b) - (b), \
					ptrdiff_t: _snow_assertneq_ptr, \
					default: _snow_assertneq_int)) \
		)(_snow_desc, _snow_spaces, _snow_name, __FILE__, #a, #b, a, b); \
		_Pragma("GCC diagnostic pop") \
		if (r < 0) \
			goto _snow_done; \
	} while (0)

#else // __STDC_VERSION__ >= 201112L

#define asserteq(a, b) _Pragma("GCC error \"asserteq requires support for C11.\"")
#define assertneq(a, b) _Pragma("GCC error \"assertneq requires support for C11.\"")

#endif

#define _snow_print_success() \
	do { \
		_snow_print_result_newline(); \
		if (_snow_opts[_snow_opt_quiet].value) break; \
		_snow_extra_newline = 1; \
		if (_snow_opts[_snow_opt_color].value) { \
			_snow_print( \
				_SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "%s✓ " \
				_SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
				_SNOW_COLOR_RESET, \
				_snow_spaces, _snow_desc); \
		} else { \
			_snow_print( \
				"%s✓ Success: %s", \
				_snow_spaces, _snow_desc); \
		} \
		_snow_print_timer(); \
	} while (0)

#define _snow_print_maybe() \
	do { \
		if (_snow_opts[_snow_opt_quiet].value) break; \
		if (!_snow_opts[_snow_opt_maybes].value) break; \
		_snow_extra_newline = 1; \
		if (_snow_opts[_snow_opt_color].value) { \
			_snow_print( \
				_SNOW_COLOR_BOLD SNOW_COLOR_MAYBE "%s? " \
				_SNOW_COLOR_RESET SNOW_COLOR_MAYBE "Testing: " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s: " \
				_SNOW_COLOR_RESET, \
				_snow_spaces, _snow_desc); \
		} else { \
			_snow_print( \
				"%s? Testing: %s: ", \
				_snow_spaces, _snow_desc); \
		} \
	} while (0)

#define _snow_print_run() \
	do { \
		if (_snow_opts[_snow_opt_quiet].value) break; \
		if (_snow_extra_newline) { \
			_snow_print("\n"); \
		} \
		if (_snow_opts[_snow_opt_color].value) { \
			_snow_print( \
				_SNOW_COLOR_BOLD "%sTesting %s:" _SNOW_COLOR_RESET "\n", \
				_snow_spaces, _snow_name); \
		} else { \
			_snow_print( \
				"%sTesting %s:\n", \
				_snow_spaces, _snow_name); \
		} \
	} while (0)

#define _snow_print_done() \
	do { \
		if (_snow_opts[_snow_opt_quiet].value) break; \
		_snow_extra_newline = 0; \
		if (_snow_opts[_snow_opt_color].value) { \
			_snow_print( \
				_SNOW_COLOR_BOLD "%s%s: Passed %i/%i tests." \
				_SNOW_COLOR_RESET "\n\n", \
				_snow_spaces, _snow_name, _snow_successes, _snow_total); \
		} else { \
			_snow_print( \
				"%s%s: Passed %i/%i tests.\n\n", \
				_snow_spaces, _snow_name, _snow_successes, _snow_total); \
		} \
	} while (0)

#define defer(...) \
	do { \
		__label__ _snow_defer_label; \
		_snow_defer_label: \
		if (_snow_rundefer) { \
			__VA_ARGS__; \
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
					_snow_labels.size = 16; \
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

#define test(testdesc, ...) \
	do { \
		__label__ _snow_done; \
		/* This is to make Clang shut up about "indirect goto in function */ \
		/* with no address-of-label expressions" when there's no defer(). */ \
		__attribute__((unused)) void *_snow_unused_label = &&_snow_done; \
		int __attribute__((unused)) _snow_rundefer = 0; \
		const char *_snow_desc = testdesc; \
		_snow_total += 1; \
		_snow_print_maybe(); \
		gettimeofday(&_snow_timer, NULL); \
		__VA_ARGS__ \
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
#define it test

#define subdesc(testname, ...) \
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
		char *_snow_spaces = (char*)malloc(_snow_depth * 2 + 1); \
		int i; \
		for (i = 0; i < _snow_depth * 2; ++i) \
			_snow_spaces[i] = ' '; \
		_snow_spaces[_snow_depth * 2] = '\0'; \
		_snow_print_run(); \
		__VA_ARGS__ \
		_snow_print_done(); \
		free(_snow_spaces); \
		*_snow_parent_successes += _snow_successes; \
		*_snow_parent_total += _snow_total; \
	} while(0)

#define describe(testname, ...) \
	static void test_##testname() { \
		_snow_num_defines += 1; \
		const char *_snow_name = #testname; \
		int __attribute__((unused)) _snow_depth = 0; \
		int _snow_successes = 0; \
		int _snow_total = 0; \
		const char *_snow_spaces = ""; \
		_snow_print_run(); \
		__VA_ARGS__ \
		_snow_print_done(); \
		_snow_global_successes += _snow_successes; \
		_snow_global_total += _snow_total; \
	} \
	__attribute__((constructor (__COUNTER__ + 101))) \
	static void _snow_constructor_##testname() { \
		_snow_describes.count += 1; \
		if (_snow_describes.count >= _snow_describes.size) { \
			if (_snow_describes.size == 0) \
				_snow_describes.size = 16; \
			else \
				_snow_describes.size *= 2; \
			_snow_describes.describes = (void (**)())realloc( \
				_snow_describes.describes, \
				_snow_describes.size * sizeof(*_snow_describes.describes)); \
		} \
		_snow_describes.describes[_snow_describes.count - 1] = \
			&test_##testname; \
	}

#define _snow_usage(argv0) \
	do { \
		_snow_print("Usage: %s [options]     Run all tests.\n", argv0); \
		_snow_print("       %s -v|--version  Print version and exit.\n", argv0); \
		_snow_print("       %s -h|--help     Display this help text and exit.\n", argv0); \
		_snow_print( \
			"\n" \
			"Arguments:\n" \
			"    --color|-c:   Enable colors.\n" \
			"                  Default: on when output is a TTY.\n" \
			"    --no-color:   Force disable --color.\n" \
			"\n" \
			"    --quiet|-q:   Suppress most messages, only test failures and a summary\n" \
			"                  error count is shown.\n" \
			"                  Default: off.\n" \
			"    --no-quiet:   Force disable --quiet.\n" \
			"\n" \
			"    --log <file>: Log output to a file, rather than stdout.\n" \
			"\n" \
			"    --timer|-t:   Display the time taken for by each test after\n" \
			"                  it is completed.\n" \
			"                  Default: on.\n" \
			"    --no-timer:   Force disable --timer.\n" \
			"\n" \
			"    --maybes|-m:  Print out messages when begining a test as well\n" \
			"                  as when it is completed.\n" \
			"                  Default: on when the output is a TTY.\n" \
			"    --no-maybes:  Force disable --maybes.\n" \
			"\n" \
			"    --cr:         Print a carriage return (\\r) rather than a newline\n" \
			"                  after each --maybes message. This means that the fail or\n" \
			"                  success message will appear on the same line.\n" \
			"                  Default: on when the output is a TTY.\n" \
			"    --no-cr:      Force disable --cr.\n"); \
	} while (0)

#define snow_main() \
	int _snow_exit_code = EXIT_SUCCESS; \
	int _snow_extra_newline = 1; \
	int _snow_global_total = 0; \
	int _snow_global_successes = 0; \
	int _snow_num_defines = 0; \
	FILE *_snow_log_file; \
	struct timeval _snow_timer; \
	struct _snow_labels _snow_labels = { NULL, 0, 0 }; \
	struct _snow_describes _snow_describes = { NULL, 0, 0 }; \
	struct _snow_option _snow_opts[] = { \
		[_snow_opt_version] = { "version", 'v',  0, 0 }, \
		[_snow_opt_help]    = { "help",    'h',  0, 0 }, \
		[_snow_opt_color]   = { "color",   'c',  1, 0 }, \
		[_snow_opt_quiet]   = { "quiet",   'q',  0, 0 }, \
		[_snow_opt_maybes]  = { "maybes",  'm',  1, 0 }, \
		[_snow_opt_cr]      = { "cr",      '\0', 1, 0 }, \
		[_snow_opt_timer]   = { "timer",   't',  1, 0 }, \
	}; \
	int main(int argc, char **argv) { \
		_snow_log_file = stdout; \
		int i; \
		for (i = 1; i < argc; ++i) { \
			int j, len; \
			len = sizeof(_snow_opts)/sizeof(*_snow_opts); \
			if (strncmp(argv[i], "--no-", 5) == 0) { \
				for (j = 0; j < len; ++j) { \
					struct _snow_option *opt = &_snow_opts[j]; \
					if (strcmp(&argv[i][5], opt->fullname) == 0) { \
						opt->value = 0; \
						opt->overridden = 1; \
					} \
				} \
			} \
			else if (strncmp(argv[i], "--", 2) == 0) { \
				for (j = 0; j < len; ++j) { \
					struct _snow_option *opt = &_snow_opts[j]; \
					if (strcmp(&argv[i][2], opt->fullname) == 0) { \
						opt->value = 1; \
						opt->overridden = 1; \
					} \
				} \
			} \
			if (strncmp(argv[i], "-", 1) == 0 && strlen(argv[i]) == 2) { \
				for (j = 0; j < len; ++j) { \
					struct _snow_option *opt = &_snow_opts[j]; \
					if (argv[i][1] == opt->shortname) { \
						opt->value = 1; \
						opt->overridden = 1; \
					} \
				} \
			} \
			if (strcmp(argv[i], "--log") == 0) { \
				if (++i >= argc) break; \
				if (strcmp(argv[i], "-") == 0) \
					_snow_log_file = stdout; \
				else \
					_snow_log_file = fopen(argv[i], "w"); \
				if (_snow_log_file == NULL) { \
					_snow_log_file = stdout; \
					_snow_print( \
						"Could not open log file '%s': %s", \
						argv[i], strerror(errno)); \
					return -1; \
				} \
			} \
		} \
		/* Default to no colors if NO_COLOR */ \
		if (getenv("NO_COLOR") != NULL) { \
			if (!_snow_opts[_snow_opt_color].overridden) \
				_snow_opts[_snow_opt_color].value = 0; \
		} \
		/* If not a tty, default to "boring" output */ \
		if (!_SNOW_ISATTY(_snow_log_file)) { \
			if (!_snow_opts[_snow_opt_color].overridden) \
				_snow_opts[_snow_opt_color].value = 0; \
			if (!_snow_opts[_snow_opt_maybes].overridden) \
				_snow_opts[_snow_opt_maybes].value = 0; \
			if (!_snow_opts[_snow_opt_cr].overridden) \
				_snow_opts[_snow_opt_cr].value = 0; \
		} \
		/* --version: Print version and exit */ \
		if (_snow_opts[_snow_opt_version].value) { \
			_snow_print("Snow %s\n", SNOW_VERSION); \
			return EXIT_SUCCESS; \
		} \
		/* --help: Print usage and exit */ \
		if (_snow_opts[_snow_opt_help].value) { \
			_snow_usage(argv[0]); \
			return EXIT_SUCCESS; \
		} \
		/* Run tests */ \
		size_t j; \
		for (j = 0; j < _snow_describes.count; ++j) { \
			_snow_describes.describes[j](); \
		} \
		/* Cleanup, print result */ \
		free(_snow_labels.labels); \
		free(_snow_describes.describes); \
		if (_snow_num_defines > 1 || _snow_opts[_snow_opt_quiet].value) { \
			if (_snow_opts[_snow_opt_color].value) { \
				_snow_print( \
					_SNOW_COLOR_BOLD "Total: Passed %i/%i tests.\n" \
					_SNOW_COLOR_RESET, \
					_snow_global_successes, _snow_global_total); \
			} else { \
				_snow_print( \
					"Total: Passed %i/%i tests.\n", \
					_snow_global_successes, _snow_global_total); \
			} \
			if (!_snow_opts[_snow_opt_quiet].value) \
				_snow_print("\n"); \
		} \
		return _snow_exit_code; \
	}

#endif // SNOW_ENABLED

#endif // SNOW_H
