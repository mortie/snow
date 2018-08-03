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
#include <unistd.h>
#include <fnmatch.h>
#include <sys/time.h>

#define SNOW_VERSION "X"

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
static void _snow_arr_reset(struct _snow_arr *arr) {
	free(arr->elems);
	arr->length = 0;
	arr->allocated = 0;
}

/*
 * More data structures
 */

struct _snow_context {
	char *name;
	int depth;
	struct _snow_context *parent;
	int num_tests;
	int num_success;
	int done;
	int in_case;
	int printed_testing;
	double start_time;
	int enabled;
};

struct _snow_case_context {
	char *name;
	char *filename;
	int linenum;
};

struct _snow_desc {
	char *name;
	void (*func)(struct _snow_context *_snow_context);
};

struct _snow_opt {
	char *name;
	char shortname;
	int is_bool;
	union {
		int boolval;
		char *strval;
	};
	int is_overwritten;
};

enum {
	_SNOW_OPT_VERSION,
	_SNOW_OPT_HELP,
	_SNOW_OPT_COLOR,
	_SNOW_OPT_QUIET,
	_SNOW_OPT_MAYBES,
	_SNOW_OPT_CR,
	_SNOW_OPT_TIMER,
	_SNOW_OPT_LOG,
	_SNOW_OPT_LAST,
};

/*
 * Global stuff
 */

extern struct _snow_arr _snow_descs;
extern jmp_buf _snow_case_start;
extern double _snow_case_start_time;
extern struct _snow_arr _snow_defers;
extern struct _snow_arr _snow_desc_patterns;
extern FILE *_snow_log_file;
extern struct _snow_opt _snow_opts[];
extern int _snow_exit_code;
extern char *_snow_desc_filter_str;
extern size_t _snow_desc_filter_str_size;

#define _snow_opt_default(opt, val) \
	if (!_snow_opts[opt].is_overwritten) _snow_opts[opt].boolval = val

/*
 * Filter descs
 */

size_t _snow_desc_filter_str_build(struct _snow_context *context) {
	if (context == NULL) {
		return 0;
	}

	size_t idx = _snow_desc_filter_str_build(context->parent);

	size_t len = strlen(context->name);
	if (_snow_desc_filter_str_size <= idx + len + 2) {
		_snow_desc_filter_str_size = idx + len + 2;
		_snow_desc_filter_str = realloc(
			_snow_desc_filter_str, _snow_desc_filter_str_size);
	}

	if (idx != 0) {
		_snow_desc_filter_str[idx++] = '.';
	}
	memcpy(_snow_desc_filter_str + idx, context->name, len);
	_snow_desc_filter_str[idx + len] = '\0';

	return idx + len;
}
int _snow_desc_filter(struct _snow_context *context, char *name) {
	if (_snow_desc_patterns.length == 0)
		return 1;

	struct _snow_context tmp = { 0 };
	tmp.parent = context;
	tmp.name = name;
	_snow_desc_filter_str_build(&tmp);

	char *string = _snow_desc_filter_str;

	for (size_t i = 0; i < _snow_desc_patterns.length; ++i) {
		char *pattern = *(char **)_snow_arr_get(&_snow_desc_patterns, i);
		int match = fnmatch(pattern, string, 0);
		if (match == 0) {
			return 1;
		} else if (match != FNM_NOMATCH) {
			fprintf(stderr, "Pattern error: %s\n", pattern);
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

/*
 * Get current time as a double
 */

double _snow_now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
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

static int _snow_need_cr = 0;

#define _snow_print(...) \
	fprintf(_snow_log_file, __VA_ARGS__)

#define _snow_printd(context, offs, ...) \
	do { \
		if (_snow_need_cr) { \
			_snow_print("\r"); \
			_snow_need_cr = 0; \
		} \
		_snow_print("%s", _snow_spaces((context)->depth + (offs))); \
		_snow_print(__VA_ARGS__); \
	} while (0)

#define _SNOW_NL_DESC 1
#define _SNOW_NL_CASE 2
#define _SNOW_NL_RES 3
static int _snow_nl = 0;

#define _snow_print_timer(start_time) \
	double msec = _snow_now() - (start_time); \
	if (msec < 1000) { \
		_snow_print("(%.02fms)", msec); \
	} else { \
		_snow_print("(%.02fs)", msec / 1000); \
	} \

#define _snow_print_testing(context, name) \
	if (!_snow_opts[_SNOW_OPT_QUIET].boolval) { \
		(context)->printed_testing = 1; \
		_snow_print("\n"); \
		_snow_nl = _SNOW_NL_DESC; \
		if (_snow_opts[_SNOW_OPT_COLOR].boolval) { \
			_snow_printd(context, -1, \
				_SNOW_COLOR_BOLD "Testing %s" _SNOW_COLOR_RESET ":\n", \
				name); \
		} else { \
			_snow_printd(context, -1, \
				"Testing %s:\n", name); \
		} \
	}

void _snow_print_testing_parents(struct _snow_context *context) {
	if (context == NULL || _snow_opts[_SNOW_OPT_QUIET].boolval)
		return;

	_snow_print_testing_parents(context->parent);

	if (!context->printed_testing) {
		_snow_print_testing(context, context->name);
		context->printed_testing = 1;
	}
}

#define _snow_print_result(context, name, passed, total) \
	if ((context)->printed_testing && !_snow_opts[_SNOW_OPT_QUIET].boolval) { \
		if (_snow_nl != _SNOW_NL_CASE) \
			_snow_print("\n"); \
		_snow_nl = _SNOW_NL_RES; \
		if (_snow_opts[_SNOW_OPT_COLOR].boolval) { \
			_snow_printd(context, -1, \
				_SNOW_COLOR_BOLD "%s: Passed %i/%i tests." \
				_SNOW_COLOR_RESET, \
				name, passed, total); \
		} else { \
			_snow_printd(context, -1, \
				"%s: Passed %i/%i tests.", \
				name, passed, total); \
		} \
		if (_snow_opts[_SNOW_OPT_TIMER].boolval) { \
			_snow_print(" "); \
			_snow_print_timer((context)->start_time); \
		} \
		_snow_print("\n"); \
	}

#define _snow_print_maybe(context, name) \
	if (!_snow_opts[_SNOW_OPT_QUIET].boolval && _snow_opts[_SNOW_OPT_MAYBES].boolval) { \
		if (_snow_opts[_SNOW_OPT_COLOR].boolval) { \
			_snow_printd(context, -1, \
				_SNOW_COLOR_BOLD SNOW_COLOR_MAYBE "? " \
				_SNOW_COLOR_RESET SNOW_COLOR_MAYBE "Testing: " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s: " _SNOW_COLOR_RESET, \
				name); \
		} else { \
			_snow_printd(context, -1, \
				"? Testing: %s: ", name); \
		} \
		if (!_snow_opts[_SNOW_OPT_CR].boolval) { \
			_snow_print("\n"); \
		} else { \
			_snow_need_cr = 1; \
			fflush(stdout); \
		} \
	}

#define _snow_print_success(context, name) \
	if (!_snow_opts[_SNOW_OPT_QUIET].boolval) { \
		_snow_nl = _SNOW_NL_CASE; \
		if (_snow_opts[_SNOW_OPT_COLOR].boolval) { \
			_snow_printd(context, -1, \
				_SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "✓ " \
				_SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: " \
				_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s " \
				_SNOW_COLOR_RESET, \
				name); \
		} else { \
			_snow_printd(context, -1, \
				"✓ Success: %s ", name); \
		} \
		if (_snow_opts[_SNOW_OPT_TIMER].boolval) { \
			_snow_print_timer(_snow_case_start_time); \
		} \
		_snow_print("\n"); \
	}

#define _snow_print_failure(context, name) \
	_snow_nl = _SNOW_NL_CASE; \
	if (_snow_opts[_SNOW_OPT_COLOR].boolval) { \
		_snow_printd(context, -1, \
			_SNOW_COLOR_BOLD SNOW_COLOR_FAIL "✕ " \
			_SNOW_COLOR_RESET SNOW_COLOR_FAIL "Failed:  " \
			_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s" \
			_SNOW_COLOR_RESET ":\n", \
			name); \
	} else { \
		_snow_printd(context, -1, \
			"✕ Failed:  %s:\n", name); \
	}

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
	for ( \
		struct _snow_context _snow_ctx = { \
				.name = #descname, \
				.depth = _snow_context->depth + 1, \
				.parent = _snow_context, \
				.num_tests = 0, \
				.num_success = 0, \
				.done = 0, \
				.printed_testing = 0, \
				.start_time = _snow_now(), \
				.enabled = \
					_snow_context->enabled || \
					_snow_desc_filter(_snow_context, #descname), \
			}, \
			*_snow_context = &_snow_ctx; \
		_snow_context->done == 0; \
		_snow_subdesc_after(_snow_context)) __VA_ARGS__

#define it(casename, ...) \
	if (_snow_context->enabled) { \
		_snow_context->num_tests += 1; \
		_snow_context->in_case = 1; \
		\
		int jmpret = setjmp(_snow_case_start); \
		if (jmpret != 0) { \
			_snow_context->in_case = 0; \
			if (jmpret == 1) { /* Test succeeded */ \
				_snow_context->num_success += 1; \
				_snow_print_success(_snow_context, casename); \
			} else if (jmpret == -1) { /* Test failed */ \
				/* We don't have to do anything more */ \
			} else { /* There are defers to run */ \
			} \
		} else { \
			_snow_case_start_time = _snow_now(); \
			if (!_snow_context->printed_testing) { \
				_snow_print_testing_parents(_snow_context); \
			} \
			_snow_print_maybe(_snow_context, casename); \
		} \
	} \
	for ( \
		struct _snow_case_context _snow_case_ctx = { casename, __FILE__, __LINE__ }, \
			__attribute__((unused)) *_snow_case_context = &_snow_case_ctx; \
		_snow_context->in_case && _snow_context->enabled; \
		longjmp(_snow_case_start, 1)) __VA_ARGS__
#define test it

/*
 * Fail a test.
 */

#define fail(...) \
	do { \
		_snow_exit_code = EXIT_FAILURE; \
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
		if ((exp)[0] == '\0') \
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

int _snow_assert_buf(
		struct _snow_context *_snow_context, struct _snow_case_context *_snow_case_context,
		int invert, char *explanation,
		void *a, char *astr, void *b, char *bstr, size_t size)
{
	int eq = memcmp(a, b, size) == 0;
	if (!eq && !invert) {
		_snow_fail_expl(explanation, "(buf) Expected %s to equal %s", \
			astr, bstr);
	} else if (eq && invert) {
		_snow_fail_expl(explanation, "(buf) Expected %s to not equal %s",
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

/*
 * Explicit asserteq macros
 */

#define asserteq_dbl(a, b, ...) \
	_snow_assert_dbl(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b)
#define asserteq_ptr(a, b, ...) \
	_snow_assert_ptr(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b)
#define asserteq_str(a, b, ...) \
	_snow_assert_str(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b)
#define asserteq_int(a, b, ...) \
	_snow_assert_int(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b)
#define asserteq_uint(a, b, ...) \
	_snow_assert_uint(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b)
#define asserteq_buf(a, b, size, ...) \
	_snow_assert_buf(_snow_context, _snow_case_context, \
	0, "" __VA_ARGS__, (a), #a, (b), #b, size)

/*
 * Explicit assertneq macros
 */

#define assernteq_dbl(a, b, ...) \
	_snow_assert_dbl(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b)
#define assernteq_ptr(a, b, ...) \
	_snow_assert_ptr(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b)
#define assernteq_str(a, b, ...) \
	_snow_assert_str(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b)
#define assernteq_int(a, b, ...) \
	_snow_assert_int(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b)
#define assernteq_uint(a, b, ...) \
	_snow_assert_uint(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b)
#define assernteq_buf(a, b, size, ...) \
	_snow_assert_buf(_snow_context, _snow_case_context, \
	1, "" __VA_ARGS__, (a), #a, (b), #b, (size))

/*
 * Automatic asserteq
 */

#define asserteq(a, b, ...) \
	do { \
		char *explanation = "" __VA_ARGS__; \
		int ret = _snow_generic_assert(b)( \
			_snow_context, _snow_case_context, 0, explanation, \
			(a), #a, (b), #b); \
		if (ret < 0) { \
			typeof (a) _a = a; \
			typeof (b) _b = b; \
			if (sizeof(a) != sizeof(b)) { \
				_snow_fail_expl(explanation, \
					"Expected %s to equal %s, but their lengths don't match", \
					#a, #b); \
			} else { \
				_snow_assert_buf( \
					_snow_context, _snow_case_context, 0, explanation, \
					&_a, #a, &_b, #b, sizeof(_a)); \
			} \
		} \
	} while (0)

/*
 * Automatic assertneq
 */

#define assertneq(a, b, ...) \
	do { \
		char *explanation = "" __VA_ARGS__; \
		if (sizeof(a) != sizeof(b)) { \
			break; \
		} else { \
			int ret = _snow_generic_assert(b)( \
				_snow_context, _snow_case_context, 1, explanation, \
				(a), #a, (b), #b); \
			if (ret < 0) { \
				typeof (a) _a = a; \
				typeof (b) _b = b; \
				if (sizeof(_a) != sizeof(_b)) { \
					break; \
				} else { \
					_snow_assert_buf( \
						_snow_context, _snow_case_context, 1, explanation, \
						&_a, #a, &_b, #b, sizeof(_a)); \
				} \
			} \
		} \
	} while (0)

/*
 * Print usage information
 */

static void _snow_usage(char *argv0)
{
	_snow_print("Usage: %s [options]            Run all tests.\n", argv0);
	_snow_print("       %s [options] <test>...  Run specific tests.\n", argv0);
	_snow_print("       %s -v|--version         Print version and exit.\n", argv0);
	_snow_print("       %s -h|--help            Display this help text and exit.\n", argv0);
	_snow_print(
		"\n"
		"Arguments:\n"
		"    --color|-c:   Enable colors.\n"
		"                  Default: on when output is a TTY.\n"
		"    --no-color:   Force disable --color.\n"
		"\n"
		"    --quiet|-q:   Suppress most messages, only test failures and a summary\n"
		"                  error count is shown.\n"
		"                  Default: off.\n"
		"    --no-quiet:   Force disable --quiet.\n"
		"\n"
		"    --log <file>: Log output to a file, rather than stdout.\n"
		"\n"
		"    --timer|-t:   Display the time taken for by each test after\n"
		"                  it is completed.\n"
		"                  Default: on.\n"
		"    --no-timer:   Force disable --timer.\n"
		"\n"
		"    --maybes|-m:  Print out messages when begining a test as well\n"
		"                  as when it is completed.\n"
		"                  Default: on when the output is a TTY.\n"
		"    --no-maybes:  Force disable --maybes.\n"
		"\n"
		"    --cr:         Print a carriage return (\\r) rather than a newline\n"
		"                  after each --maybes message. This means that the fail or\n"
		"                  success message will appear on the same line.\n"
		"                  Default: on when the output is a TTY.\n"
		"    --no-cr:      Force disable --cr.\n");
}

/*
 * Main function.
 */

int _snow_main(int argc, char **argv) {

	// Arg parsing
	int opts_done = 0;
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];

		// Parse an option
		if (!opts_done && arg[0] == '-') {
			if (strcmp(arg, "--") == 0) {
				opts_done = 1;
				continue;
			}

			// Is it a short option? What's the name without - or --?
			// Is it inverted (with a --no-)?
			int shortopt = arg[1] != '-';
			char *name = shortopt ? arg + 1 : arg + 2;
			int inverted = !shortopt && strncmp(name, "no-", 3) == 0;
			if (inverted) name += 3;

			// Find a matching _snow_opt
			int is_match = 0;
			for (size_t j = 0; j < _SNOW_OPT_LAST; ++j) {
				struct _snow_opt *opt = _snow_opts + j;
				is_match = shortopt
					? name[0] == opt->shortname
					: strcmp(name, opt->name) == 0;

				if (!is_match)
					continue;

				opt->is_overwritten = 1;
				if (opt->is_bool) {
					opt->boolval = inverted == 0;
				} else {
					if (inverted) {
						is_match = 0;
						break;
					}

					if (i + 1 >= argc ) {
						fprintf(stderr, "%s: Argument expected.\n", arg);
						return EXIT_FAILURE;
					}

					opt->strval = argv[++i];
				}

				break;
			}

			if (!is_match) {
				fprintf(stderr, "Unknown option: %s\n", arg);
				return EXIT_FAILURE;
			}

		// Add to the list of patterns if it's not an option
		} else {
			_snow_arr_append(&_snow_desc_patterns, &arg);
		}
	}

	printf(
		"version: %i\n"
		"help: %i\n"
		"color: %i\n"
		"quiet: %i\n"
		"maybes: %i\n"
		"cr: %i\n"
		"timer: %i\n"
		"log: %s\n",
		_snow_opts[_SNOW_OPT_VERSION].boolval,
		_snow_opts[_SNOW_OPT_HELP].boolval,
		_snow_opts[_SNOW_OPT_COLOR].boolval,
		_snow_opts[_SNOW_OPT_QUIET].boolval,
		_snow_opts[_SNOW_OPT_MAYBES].boolval,
		_snow_opts[_SNOW_OPT_CR].boolval,
		_snow_opts[_SNOW_OPT_TIMER].boolval,
		_snow_opts[_SNOW_OPT_LOG].strval);

	// Open log file
	if (strcmp(_snow_opts[_SNOW_OPT_LOG].strval, "-") == 0) {
		_snow_log_file = stdout;
	} else {
		_snow_log_file = fopen(_snow_opts[_SNOW_OPT_LOG].strval, "w");
		if (_snow_log_file == NULL) {
			perror(_snow_opts[_SNOW_OPT_LOG].strval);
			return EXIT_FAILURE;
		}
	}

	// --help and --version
	if (_snow_opts[_SNOW_OPT_HELP].boolval) {
		_snow_usage(argv[0]);
		return EXIT_SUCCESS;
	} else if (_snow_opts[_SNOW_OPT_VERSION].boolval) {
		_snow_print("Snow %s\n", SNOW_VERSION);
		return EXIT_SUCCESS;
	}

	// Set context-dependent defaults
	// (the dumb defaults were set in the snow_main macro)
	if (getenv("NO_COLOR") != NULL) {
		_snow_opt_default(_SNOW_OPT_COLOR, 0);
	}
	int is_tty = isatty(fileno(_snow_log_file));
	if (!is_tty) {
		_snow_opt_default(_SNOW_OPT_COLOR, 0);
		_snow_opt_default(_SNOW_OPT_MAYBES, 0);
		_snow_opt_default(_SNOW_OPT_CR, 0);
	}

	struct _snow_context root_context = { 0 };
	root_context.start_time = _snow_now();
	root_context.enabled = 1;
	root_context.printed_testing = 1;
	root_context.name = "Total";

	struct _snow_context context = { 0 };
	context.depth = 1;

	for (size_t i = 0; i < _snow_descs.length; i += 1) {
		struct _snow_desc *desc = _snow_arr_get(&_snow_descs, i);

		context.name = "";
		context.start_time = _snow_now();
		context.enabled = _snow_desc_filter(&context, desc->name);
		context.printed_testing = 0;
		context.name = desc->name;

		desc->func(&context);
		_snow_print_result(&context, context.name,
			context.num_success, context.num_tests);
		root_context.num_tests += context.num_tests;
		root_context.num_success += context.num_success;
	}

	if (_snow_opts[_SNOW_OPT_QUIET].boolval) {
		_snow_opts[_SNOW_OPT_QUIET].boolval = 0;
		_snow_print_result(&root_context, root_context.name,
			root_context.num_success, root_context.num_tests);
	} else {
		_snow_print_result(&root_context, root_context.name,
			root_context.num_success, root_context.num_tests);
		_snow_print("\n");
	}

	free(_snow_desc_filter_str);
	_snow_arr_reset(&_snow_descs);
	_snow_arr_reset(&_snow_defers);
	_snow_arr_reset(&_snow_desc_patterns);

	return _snow_exit_code;
}

/*
 * Initialize the variables which other translation units will use as extern,
 * then just run _snow_main.
 */

#define snow_main() \
	struct _snow_arr _snow_descs = { sizeof(struct _snow_desc), NULL, 0, 0 }; \
	jmp_buf _snow_case_start; \
	double _snow_case_start_time; \
	struct _snow_arr _snow_defers = { sizeof(jmp_buf), NULL, 0, 0 }; \
	struct _snow_arr _snow_desc_patterns = { sizeof(char *), NULL, 0, 0 }; \
	FILE *_snow_log_file = NULL; \
	int _snow_exit_code = EXIT_SUCCESS; \
	char *_snow_desc_filter_str = NULL; \
	size_t _snow_desc_filter_str_size = 0; \
	struct _snow_opt _snow_opts[] = { \
		[ _SNOW_OPT_VERSION ] = { "version", 'v',  1, .boolval = 0,   0 }, \
		[ _SNOW_OPT_HELP ]    = { "help",    'h',  1, .boolval = 0,   0 }, \
		[ _SNOW_OPT_COLOR ]   = { "color",   'c',  1, .boolval = 1,   0 }, \
		[ _SNOW_OPT_QUIET ]   = { "quiet",   'q',  1, .boolval = 0,   0 }, \
		[ _SNOW_OPT_MAYBES ]  = { "maybes",  'm',  1, .boolval = 1,   0 }, \
		[ _SNOW_OPT_CR ]      = { "cr",      '\0', 1, .boolval = 1,   0 }, \
		[ _SNOW_OPT_TIMER ]   = { "timer",   't',  1, .boolval = 1,   0 }, \
		[ _SNOW_OPT_LOG ]     = { "log",     'l',  0, .strval  = "-", 0 }, \
	}; \
	int main(int argc, char **argv) { \
		return _snow_main(argc, argv); \
	}

#endif
