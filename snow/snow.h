#ifdef SNOW_ENABLED

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <setjmp.h>
#include <unistd.h>
#include <fnmatch.h>

#define SNOW_VERSION "X"

#define SNOW_MAX_DEPTH 128

/*
 * Colors
 */

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

struct _snow_desc {
	const char *name;
	char *full_name;
	size_t full_name_len;
	double start_time;
	int num_tests;
	int num_success;
	int enabled;
};

struct _snow_desc_func {
	char *name;
	void (*func)();
};

struct _snow {
	int exit_code;
	struct _snow_arr desc_funcs;
	struct _snow_arr desc_stack;
	struct _snow_arr desc_patterns;
	struct _snow_desc *current_desc;
	struct _snow_opt opts[_SNOW_OPT_LAST];

	int in_case;
	struct {
		const char *name;
		double start_time;
		struct _snow_arr defers;
		jmp_buf done_jmp;
		jmp_buf defer_jmp;
	} current_case;

	struct {
		FILE *file;
		int need_cr;
	} print;
};

/*
 * Globals
 */

extern struct _snow _snow;
extern int _snow_inited;

/*
 * Opts
 */

#define _snow_opt_default(opt, val) \
	if (!_snow.opts[opt].is_overwritten) _snow.opts[opt].boolval = val
#define _snow_opt_boolt(id, n, sn) \
	_snow.opts[id].name = n; _snow.opts[id].shortname = sn; \
	_snow.opts[id].is_bool = 1; _snow.opts[id].boolval = 0; \
	_snow.opts[id].is_overwritten = 0
#define _snow_opt_str(id, n, sn, val) \
	_snow.opts[id].name = n; _snow.opts[id].shortname = sn; \
	_snow.opts[id].is_bool = 0; _snow.opts[id].strval = val; \
	_snow.opts[id].is_overwritten = 0

/*
 * Util
 */

static char _snow_spaces_str[SNOW_MAX_DEPTH * 2 + 1];
static int _snow_spaces_depth_prev = 0;
static char *_snow_spaces(int depth) {
	if (depth > SNOW_MAX_DEPTH)
		depth = SNOW_MAX_DEPTH;

	_snow_spaces_str[depth * 2] = '\0';
	if (depth > _snow_spaces_depth_prev)
		memset(_snow_spaces_str, ' ', depth * 2);
	_snow_spaces_depth_prev = depth;

	return _snow_spaces_str;
}

static double _snow_now() {
	if (_snow.opts[_SNOW_OPT_TIMER].boolval)
		return 0;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/*
 * Printing
 */

#define _snow_print(...) fprintf(_snow.print.file, __VA_ARGS__)

static void _snow_print_timer(double start_time) {
	double msec = _snow_now() - start_time;
	if (msec < 1000) {
		_snow_print("(%.02fms)", msec);
	} else {
		_snow_print("(%.02fs)", msec / 1000);
	}
}

static void _snow_print_maybe() {
	char *spaces = _snow_spaces(_snow.desc_stack.length);

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" _SNOW_COLOR_BOLD SNOW_COLOR_MAYBE "? "
			_SNOW_COLOR_RESET SNOW_COLOR_MAYBE "Testing: "
			_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s: " _SNOW_COLOR_RESET,
			spaces, _snow.current_desc->name);
	} else {
		_snow_print(
			"%s? Testing: %s: ", spaces, _snow.current_desc->name);
	}

	if (_snow.opts[_SNOW_OPT_CR].boolval) {
		_snow.print.need_cr = 1;
		fflush(_snow.print.file);
	} else {
		_snow_print("\n");
	}
}

static void _snow_print_success() {
	char *spaces = _snow_spaces(_snow.desc_stack.length);

	if (_snow.print.need_cr)
		_snow_print("\r");

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" _SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "✓ "
			_SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: "
			_SNOW_COLOR_RESET SNOW_COLOR_DESC "%s "
			_SNOW_COLOR_RESET,
			spaces, _snow.current_case.name);
	} else {
		_snow_print(
			"✓ Success: %s ", _snow.current_desc->name);
	}

	if (_snow.opts[_SNOW_OPT_TIMER].boolval) {
		_snow_print_timer(_snow.current_case.start_time);
	}

	_snow_print("\n");
}

static void _snow_print_testing() {
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" _SNOW_COLOR_BOLD "Testing %s" _SNOW_COLOR_RESET ":\n",
			spaces, _snow.current_desc->name);
	} else {
		_snow_print("%sTesting %s:\n", spaces, _snow.current_desc->name);
	}
}

void _snow_print_result() {
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" _SNOW_COLOR_BOLD "%s: Passed %i/%i tests."
			_SNOW_COLOR_RESET,
			spaces, _snow.current_desc->name,
			_snow.current_desc->num_success, _snow.current_desc->num_tests);
	} else {
		_snow_print(
			"%s%s: Passed %i/%i tests.",
			spaces, _snow.current_desc->name,
			_snow.current_desc->num_success, _snow.current_desc->num_tests);
	}

	if (_snow.opts[_SNOW_OPT_TIMER].boolval) {
		_snow_print(" ");
		_snow_print_timer(_snow.current_desc->start_time);
	}

	_snow_print("\n");
}

/*
 * Default _snow_before_each and _snow_after_each functions
 * which just do nothing.
 */
__attribute__((unused)) void _snow_before_each() {}
__attribute__((unused)) void _snow_after_each() {}

static void _snow_init() {
	_snow_inited = 1;
	_snow.exit_code = EXIT_SUCCESS;
	_snow_arr_init(&_snow.desc_funcs, sizeof(struct _snow_desc_func));
	_snow_arr_init(&_snow.desc_stack, sizeof(struct _snow_desc));
	_snow_arr_init(&_snow.desc_patterns, sizeof(char *));
	_snow_arr_init(&_snow.current_case.defers, sizeof(jmp_buf));
	_snow.current_desc = NULL;

	_snow_opt_boolt(_SNOW_OPT_VERSION, "version", 'v');
	_snow_opt_boolt(_SNOW_OPT_HELP,    "help",    'h');
	_snow_opt_boolt(_SNOW_OPT_COLOR,   "color",   'c');
	_snow_opt_boolt(_SNOW_OPT_QUIET,   "quiet",   'q');
	_snow_opt_boolt(_SNOW_OPT_MAYBES,  "maybes",  'm');
	_snow_opt_boolt(_SNOW_OPT_CR,      "cr",      '\0');
	_snow_opt_boolt(_SNOW_OPT_TIMER,   "timer",   't');

	_snow_opt_str(_SNOW_OPT_LOG, "log", 'l', "-");

	_snow.in_case = 0;

	_snow.print.file = stdout;
}

__attribute__((unused))
static void _snow_desc_begin(const char *name) {
	struct _snow_desc desc = { 0 };
	desc.name = name;
	desc.start_time = _snow_now();

	struct _snow_desc *parent_desc = NULL;
	if (_snow.desc_stack.length > 0)
		parent_desc = (struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);

	// Create the full name
	if (parent_desc == NULL) {
		desc.full_name_len = strlen(name);
		desc.full_name = malloc(desc.full_name_len + 1);
		strcpy(desc.full_name, name);
	} else {
		desc.full_name_len = strlen(name) + parent_desc->full_name_len;
		desc.full_name = malloc(desc.full_name_len + 2);
		strcpy(desc.full_name, parent_desc->full_name);
		strcpy(desc.full_name + parent_desc->full_name_len, ".");
		strcpy(desc.full_name + parent_desc->full_name_len + 1, name);
	}

	// Check if desc is enabled
	if (_snow.desc_patterns.length == 0) {
		desc.enabled = 1;
	} else if (parent_desc && parent_desc->enabled) {
		desc.enabled = 1;
	} else {
		desc.enabled = 0;

		for (size_t i = 0; i < _snow.desc_patterns.length; ++i) {
			char *pattern = *(char **)_snow_arr_get(&_snow.desc_patterns, i);
			int match = fnmatch(pattern, desc.full_name, 0);
			if (match == 0) {
				desc.enabled = 1;
				break;
			} else if (match != FNM_NOMATCH) {
				fprintf(stderr, "Pattern error: %s\n", pattern);
				exit(EXIT_FAILURE);
			}
		}
	}

	_snow_arr_push(&_snow.desc_stack, &desc);

	_snow.current_desc =
		(struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);

	_snow_print_testing();
}

__attribute__((unused))
static void _snow_desc_end() {
	_snow_print_result();

	struct _snow_desc *desc =
		(struct _snow_desc *)_snow_arr_pop(&_snow.desc_stack);

	if (_snow.desc_stack.length > 0) {
		_snow.current_desc =
			(struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);

		_snow.current_desc->num_tests += desc->num_tests;
		_snow.current_desc->num_success += desc->num_success;
	} else {
		_snow.current_desc = NULL;
	}

	free(desc->full_name);
}

/*
 * Begin a test case. It has to be a macro, not a function, because
 * longjmp can't jump to setjmps from a function call which has returned.
 */
#define _snow_case_begin(casename, before, after) \
	do { \
		if (!_snow.current_desc->enabled) break; \
		_snow.in_case = 1; \
		_snow.current_case.name = casename; \
		_snow.current_case.start_time = _snow_now(); \
		_snow_arr_reset(&_snow.current_case.defers); \
		_snow_print_maybe(); \
		_snow.current_desc->num_tests += 1; \
		before(); \
		/* Set jump point which _snow_case_end */ \
		/* (and each defer) will jump back to */ \
		if (setjmp(_snow.current_case.done_jmp) == 1) { \
			while (_snow.current_case.defers.length > 0) { \
				if (setjmp(_snow.current_case.defer_jmp) == 0) { \
					jmp_buf *jmp = (jmp_buf *)_snow_arr_pop(&_snow.current_case.defers); \
					longjmp(*jmp, 1); \
				} \
			} \
			after(); \
		} \
	} while (0)

/*
 * Called after a test case block is done.
 */
__attribute__((unused))
static void _snow_case_end(int success) {
	if (!_snow.in_case)
		return;

	_snow.in_case = 0;
	if (success) {
		_snow.current_desc->num_success += 1;
		_snow_print_success();
	}

	longjmp(_snow.current_case.done_jmp, 1);
}

/*
 * Called by defer, to register a new jmp_buf
 * on the defer stack.
 */
__attribute__((unused))
static void _snow_case_defer_push(jmp_buf jmp) {
	_snow_arr_push(&_snow.current_case.defers, jmp);
}

/*
 * Called when a defer is done.
 * Will jump back to _snow_case_begin.
 */
__attribute__((unused))
static void _snow_case_defer_jmp() {
	longjmp(_snow.current_case.defer_jmp, 1);
}

/*
 * Usage
 */
__attribute__((unused))
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
 * The main function, which runs all top-level describes
 * and cleans up.
 */
__attribute__((unused))
static int _snow_main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	/*
	 * Parse arguments
	 */

	int opts_done = 0;
	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];

		if (opts_done || arg[0] != '-') {
			_snow_arr_push(&_snow.desc_patterns, &arg);
			continue;
		}

		if (strcmp(arg, "--") == 0) {
			opts_done = 1;
			continue;
		}

		int is_long = arg[1] == '-';
		char *name = is_long ? arg + 2 : arg + 1;
		int inverted = is_long && strncmp(name, "no-", 3) == 0;
		if (inverted) name += 3;

		int is_match = 0;
		for (int i = 0; i < _SNOW_OPT_LAST; ++i) {
			struct _snow_opt *opt = _snow.opts + i;
			is_match = is_long ? strcmp(name, opt->name) == 0 : name[0] == opt->shortname;
			if (!is_match) continue;

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
	}

	/*
	 * Respond to args
	 */

	// Open log file
	if (strcmp(_snow.opts[_SNOW_OPT_LOG].strval, "-") == 0) {
		_snow.print.file = stdout;
	} else {
		_snow.print.file = fopen(_snow.opts[_SNOW_OPT_LOG].strval, "w");
		if (_snow.print.file == NULL) {
			perror(_snow.opts[_SNOW_OPT_LOG].strval);
			return EXIT_FAILURE;
		}
	}

	// --help and --version
	if (_snow.opts[_SNOW_OPT_HELP].boolval) {
		_snow_usage(argv[0]);
		return EXIT_SUCCESS;
	} else if (_snow.opts[_SNOW_OPT_VERSION].boolval) {
		_snow_print("Snow %s\n", SNOW_VERSION);
		return EXIT_SUCCESS;
	}

	// Set context-dependent defaults
	// (the dumb defaults were set in the snow_main macro)
	if (getenv("NO_COLOR") != NULL) {
		_snow_opt_default(_SNOW_OPT_COLOR, 0);
	}
	int is_tty = isatty(fileno(_snow.print.file));
	if (is_tty) {
		_snow_opt_default(_SNOW_OPT_COLOR, 1);
		_snow_opt_default(_SNOW_OPT_MAYBES, 1);
		_snow_opt_default(_SNOW_OPT_CR, 1);
	} else {
		_snow_opt_default(_SNOW_OPT_COLOR, 0);
		_snow_opt_default(_SNOW_OPT_MAYBES, 0);
		_snow_opt_default(_SNOW_OPT_CR, 0);
	}

	// Other defaults
	_snow_opt_default(_SNOW_OPT_QUIET, 0);
	_snow_opt_default(_SNOW_OPT_TIMER, 1);

	/*
	 * Run descs
	 */

	for (size_t i = 0; i < _snow.desc_funcs.length; ++i) {
		struct _snow_desc_func *df = _snow_arr_get(&_snow.desc_funcs, i);
		_snow_desc_begin(df->name);
		df->func();
		_snow_desc_end();
	}

	/*
	 * Cleanup
	 */

	_snow_arr_reset(&_snow.desc_funcs);
	_snow_arr_reset(&_snow.desc_stack);
	_snow_arr_reset(&_snow.desc_patterns);
	_snow_arr_reset(&_snow.current_case.defers);

	return _snow.exit_code;
}

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
	for (int _snow_desc_done = 0; _snow_desc_done == 0; \
		(_snow_desc_done = 1, _snow_desc_end()))

#define it(name) \
	_snow_case_begin(name, _snow_before_each, _snow_after_each); \
	for (; _snow.in_case; _snow_case_end(1))
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

#define before_each() \
	void _snow_before_each()
#define after_each() \
	void _snow_after_each()

#define snow_main() \
	struct _snow _snow; \
	int _snow_inited = 0;
	int main(int argc, char **argv) { \
		return _snow_main(argc, argv); \
	}

#endif
