/* Snow, a testing library - https://github.com/mortie/snow */

/*
 * Copyright (c) 2018 Martin Dørum
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SNOW_H
#define SNOW_H

#ifndef SNOW_ENABLED

#define describe(name) __attribute__((unused)) static void _snow_unused_##name()
#define subdesc(...) while (0)
#define it(...) while (0)
#define test(...) while (0)
#define defer(...)
#define before_each(...) while (0)
#define after_each(...) while (0)
#define snow_fail_update(...)
#define snow_fail(...)
#define fail(...)
#define assert(...)
#define snow_break()
#define snow_rerun_failed()

#define asserteq_dbl(...)
#define asserteq_ptr(...)
#define asserteq_str(...)
#define asserteq_int(...)
#define asserteq_uint(...)
#define asserteq_buf(...)
#define asserteq(...)

#define assertneq_dbl(...)
#define assertneq_ptr(...)
#define assertneq_str(...)
#define assertneq_int(...)
#define assertneq_uint(...)
#define assertneq_buf(...)
#define assertneq(...)

#else

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdint.h>

#ifdef __MINGW32__
# ifndef SNOW_USE_FNMATCH
#  define SNOW_USE_FNMATCH 0
# endif
# ifndef SNOW_USE_FORK
#  define SNOW_USE_FORK 0
# endif
#else
# ifndef SNOW_USE_FNMATCH
#  define SNOW_USE_FNMATCH 1
# endif
# ifndef SNOW_USE_FORK
#  define SNOW_USE_FORK 1
# endif
#endif

#if SNOW_USE_FNMATCH != 0
#include <fnmatch.h>
#endif

#if SNOW_USE_FORK != 0
#include <sys/wait.h>
#endif

#define SNOW_VERSION "2.3.2"

// Eventually, I want to re-implement optional explanation arguments
// for assert macros to make this unnecessary.
#pragma GCC system_header
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

// This is necessary unless I can avoid using `typeof`
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"

/*
 * Colors
 */

#ifndef SNOW_COLOR_BOLD
#define SNOW_COLOR_BOLD "\033[1m"
#endif

#ifndef SNOW_COLOR_RESET
#define SNOW_COLOR_RESET "\033[0m"
#endif

#ifndef SNOW_COLOR_SUCCESS
#define SNOW_COLOR_SUCCESS "\033[32m"
#endif

#ifndef SNOW_COLOR_FAIL
#define SNOW_COLOR_FAIL "\033[31m"
#endif

#ifndef SNOW_COLOR_MAYBE
#define SNOW_COLOR_MAYBE "\033[35m"
#endif

#ifndef SNOW_COLOR_DESC
#define SNOW_COLOR_DESC SNOW_COLOR_BOLD "\033[33m"
#endif

#ifndef SNOW_DEFAULT_ARGS
#define SNOW_DEFAULT_ARGS
#endif

/*
 * Array
 */

struct _snow_arr {
	size_t elem_size;
	char *elems;
	size_t length;
	size_t allocated;
};

__attribute__((unused))
static void _snow_arr_init(struct _snow_arr *arr, size_t size) {
	arr->elem_size = size;
	arr->elems = NULL;
	arr->length = 0;
	arr->allocated = 0;
}

__attribute__((unused))
static void _snow_arr_grow(struct _snow_arr *arr, size_t size) {
	if (arr->allocated >= size)
		return;

	arr->allocated = size;
	arr->elems = realloc(arr->elems, arr->allocated * arr->elem_size);
}

__attribute__((unused))
static void *_snow_arr_get(struct _snow_arr *arr, size_t index) {
	return arr->elems + arr->elem_size * index;
}

__attribute__((unused))
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

__attribute__((unused))
static void *_snow_arr_pop(struct _snow_arr *arr) {
	void *elem = _snow_arr_get(arr, arr->length - 1);
	arr->length -= 1;
	return elem;
}

__attribute__((unused))
static void *_snow_arr_top(struct _snow_arr *arr) {
	return _snow_arr_get(arr, arr->length - 1);
}

__attribute__((unused))
static void _snow_arr_reset(struct _snow_arr *arr) {
	free(arr->elems);
	arr->elems = NULL;
	arr->length = 0;
	arr->allocated = 0;
}

/*
 * Snow Core
 */

void snow_break(void);
void snow_rerun_failed(void);

enum {
	_SNOW_OPT_VERSION,
	_SNOW_OPT_HELP,
	_SNOW_OPT_LIST,
	_SNOW_OPT_COLOR,
	_SNOW_OPT_QUIET,
	_SNOW_OPT_MAYBES,
	_SNOW_OPT_CR,
	_SNOW_OPT_TIMER,
	_SNOW_OPT_LOG,
	_SNOW_OPT_RERUN_FAILED,
	_SNOW_OPT_GDB,
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
	int printed;
	jmp_buf before_jmp;
	int has_before_jmp;
	jmp_buf after_jmp;
	int has_after_jmp;
};

struct _snow_desc_func {
	const char *name;
	void (*func)(void);
};

struct _snow {
	int exit_code;
	const char *filename;
	int linenum;

	struct _snow_arr desc_funcs;
	struct _snow_arr desc_stack;
	struct _snow_arr desc_patterns;
	struct _snow_desc *current_desc;
	struct _snow_opt opts[_SNOW_OPT_LAST];

	int in_case;
	int in_before_each;
	int in_after_each;
	int rerunning_case;
	struct {
		int success;
		const char *name;
		double start_time;
		struct _snow_arr defers;
		jmp_buf rerun;
		jmp_buf done_jmp_ret;
		jmp_buf defer_jmp_ret;
		jmp_buf before_jmp_ret;
		jmp_buf after_jmp_ret;
	} current_case;

	struct {
		FILE *file;
		int file_opened;
		int need_cr;
		enum {
			_SNOW_PRINT_CASE,
			_SNOW_PRINT_DESC_BEGIN,
			_SNOW_PRINT_DESC_END,
		} prev_print;
	} print;

	struct {
		struct _snow_arr spaces;
	} bufs;
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
#define _snow_opt_bool(id, n, sn) \
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

static int _snow_spaces_depth_prev = -1;
__attribute__((unused))
static char *_snow_spaces(int depth) {
	if (depth != _snow_spaces_depth_prev) {
		_snow_arr_grow(&_snow.bufs.spaces, depth * 2 + 1);
		_snow.bufs.spaces.elems[depth * 2] = '\0';
		memset(_snow.bufs.spaces.elems, ' ', depth * 2);
		_snow_spaces_depth_prev = depth;
	}

	return _snow.bufs.spaces.elems;
}

#ifndef SNOW_DUMMY_TIMER
__attribute__((unused))
static double _snow_now(void) {
	if (!_snow.opts[_SNOW_OPT_TIMER].boolval)
		return 0;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
#else
__attribute__((unused))
static double _snow_now(void) {
	static double time;
	time += 1000;
	return time;
}
#endif

/*
 * Printing
 */

#define _snow_print(...) fprintf(_snow.print.file, __VA_ARGS__)

__attribute__((unused))
static void _snow_print_timer(double start_time) {
	double msec = _snow_now() - start_time;
	if (msec < 0) msec = 0;
	if (msec < 1) {
		_snow_print("(%.02fµs)", msec * 1000);
	} else if (msec < 1000) {
		_snow_print("(%.02fms)", msec);
	} else {
		_snow_print("(%.02fs)", msec / 1000);
	}
}

__attribute__((unused))
static void _snow_print_case_begin(void) {
	if (_snow.opts[_SNOW_OPT_QUIET].boolval) return;
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	_snow.print.prev_print = _SNOW_PRINT_CASE;

	if (!_snow.opts[_SNOW_OPT_MAYBES].boolval)
		return;

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" SNOW_COLOR_BOLD SNOW_COLOR_MAYBE "? "
			SNOW_COLOR_RESET SNOW_COLOR_MAYBE "Testing: "
			SNOW_COLOR_RESET SNOW_COLOR_DESC "%s: " SNOW_COLOR_RESET,
			spaces, _snow.current_case.name);
	} else {
		_snow_print(
			"%s? Testing: %s: ", spaces, _snow.current_case.name);
	}

	if (_snow.opts[_SNOW_OPT_CR].boolval) {
		_snow.print.need_cr = 1;
		fflush(_snow.print.file);
	} else {
		_snow_print("\n");
	}
}

__attribute__((unused))
static void _snow_print_case_success(void) {
	if (_snow.opts[_SNOW_OPT_QUIET].boolval) return;
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	if (_snow.print.need_cr)
		_snow_print(" \r");

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" SNOW_COLOR_BOLD SNOW_COLOR_SUCCESS "✓ "
			SNOW_COLOR_RESET SNOW_COLOR_SUCCESS "Success: "
			SNOW_COLOR_RESET SNOW_COLOR_DESC "%s"
			SNOW_COLOR_RESET,
			spaces, _snow.current_case.name);
	} else {
		_snow_print(
			"%s✓ Success: %s", spaces, _snow.current_case.name);
	}

	if (_snow.opts[_SNOW_OPT_TIMER].boolval) {
		_snow_print(" ");
		_snow_print_timer(_snow.current_case.start_time);
	}

	_snow_print("\n");
}

__attribute__((unused))
static char *_snow_print_case_failure(void) {
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	if (_snow.print.need_cr)
		_snow_print(" \r");

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" SNOW_COLOR_BOLD SNOW_COLOR_FAIL "✕ "
			SNOW_COLOR_RESET SNOW_COLOR_FAIL "Failed:  "
			SNOW_COLOR_RESET SNOW_COLOR_DESC "%s"
			SNOW_COLOR_RESET ":\n",
			spaces, _snow.current_case.name);
	} else {
		_snow_print(
			"%s✕ Failed:  %s:\n", spaces, _snow.current_case.name);
	}

	return spaces;
}

__attribute__((unused))
static void _snow_print_desc_begin_index(size_t index) {
	if (index > 0) {
		struct _snow_desc *parent = _snow_arr_get(&_snow.desc_stack, index - 1);
		if (!parent->printed)
			_snow_print_desc_begin_index(index - 1);
	}

	_snow_print("\n");
	_snow.print.prev_print = _SNOW_PRINT_DESC_BEGIN;

	struct _snow_desc *desc = _snow_arr_get(&_snow.desc_stack, index);
	char *spaces = _snow_spaces(index);

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" SNOW_COLOR_BOLD "Testing %s" SNOW_COLOR_RESET ":\n",
			spaces, desc->name);
	} else {
		_snow_print("%sTesting %s:\n", spaces, desc->name);
	}

	desc->printed = 1;
}

__attribute__((unused))
static void _snow_print_desc_begin(void) {
	if (_snow.opts[_SNOW_OPT_QUIET].boolval) return;
	_snow_print_desc_begin_index(_snow.desc_stack.length - 1);
}

__attribute__((unused))
static void _snow_print_desc_end(void) {
	if (_snow.opts[_SNOW_OPT_QUIET].boolval) return;
	char *spaces = _snow_spaces(_snow.desc_stack.length - 1);

	if (_snow.print.prev_print != _SNOW_PRINT_CASE)
		_snow_print("\n");
	_snow.print.prev_print = _SNOW_PRINT_DESC_END;

	if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
		_snow_print(
			"%s" SNOW_COLOR_BOLD "%s: Passed %i/%i tests."
			SNOW_COLOR_RESET,
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
 * Failing
 */

#define snow_fail(...) \
	if (_snow.rerunning_case) { \
		snow_rerun_failed(); \
	} \
	do { \
		char *spaces = _snow_print_case_failure(); \
		_snow_print("%s    ", spaces); \
		_snow_print(__VA_ARGS__); \
		_snow_print("\n"); \
		_snow_print("%s    in %s:%i(%s)\n", spaces, \
			_snow.filename, _snow.linenum, _snow.current_desc->full_name); \
		_snow_case_end(0); \
	} while (0)

#define _snow_fail_expl(expl, fmt, ...) \
	do { \
		if (expl[0] == '\0') { \
			snow_fail(fmt ".", __VA_ARGS__); \
		} else { \
			snow_fail(fmt ": %s", __VA_ARGS__, expl); \
		} \
	} while (0)

#define snow_fail_update() \
	do { \
		_snow.filename = __FILE__; \
		_snow.linenum = __LINE__; \
	} while (0)

__attribute__((unused))
static void _snow_init(void) {
	_snow_inited = 1;
	memset(&_snow, 0, sizeof(_snow));
	_snow.exit_code = EXIT_SUCCESS;
	_snow_arr_init(&_snow.desc_funcs, sizeof(struct _snow_desc_func));
	_snow_arr_init(&_snow.desc_stack, sizeof(struct _snow_desc));
	_snow_arr_init(&_snow.desc_patterns, sizeof(char *));
	_snow_arr_init(&_snow.current_case.defers, sizeof(jmp_buf));
	_snow_arr_init(&_snow.bufs.spaces, sizeof(char));
	_snow.current_desc = NULL;

	_snow_opt_bool(_SNOW_OPT_VERSION,      "version",      'v');
	_snow_opt_bool(_SNOW_OPT_HELP,         "help",         'h');
	_snow_opt_bool(_SNOW_OPT_LIST,         "list",         'l');
	_snow_opt_bool(_SNOW_OPT_COLOR,        "color",        'c');
	_snow_opt_bool(_SNOW_OPT_QUIET,        "quiet",        'q');
	_snow_opt_bool(_SNOW_OPT_MAYBES,       "maybes",       'm');
	_snow_opt_bool(_SNOW_OPT_CR,           "cr",           '\0');
	_snow_opt_bool(_SNOW_OPT_TIMER,        "timer",        't');
	_snow_opt_bool(_SNOW_OPT_RERUN_FAILED, "rerun-failed", '\0');
	_snow_opt_bool(_SNOW_OPT_GDB,          "gdb",          'g');

	_snow_opt_str(_SNOW_OPT_LOG, "log", 'l', "-");

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
		desc.full_name_len = strlen(name) + parent_desc->full_name_len + 1;
		desc.full_name = malloc(desc.full_name_len + 2);
		strcpy(desc.full_name, parent_desc->full_name);
		strcpy(desc.full_name + parent_desc->full_name_len, ".");
		strcpy(desc.full_name + parent_desc->full_name_len + 1, name);
		desc.has_before_jmp = parent_desc->has_before_jmp;
		if (desc.has_before_jmp)
			memcpy(desc.before_jmp, parent_desc->before_jmp, sizeof(parent_desc->before_jmp));
		desc.has_after_jmp = parent_desc->has_after_jmp;
		if (desc.has_after_jmp)
			memcpy(desc.after_jmp, parent_desc->after_jmp, sizeof(parent_desc->after_jmp));
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
			int matched = 0;
			int error = 0;

			// Use fnmatch to do glob matching if that's enabled,
			// otherwise just compare with strcmp
#if SNOW_USE_FNMATCH != 0
			int fm = fnmatch(pattern, desc.full_name, 0);
			matched = fm == 0;
			error = !matched && fm != FNM_NOMATCH;
#else
			matched = strcmp(pattern, desc.full_name) == 0;
#endif

			if (matched) {
				desc.enabled = 1;
				break;
			} else if (error) {
				fprintf(stderr, "Pattern error: %s\n", pattern);
				exit(EXIT_FAILURE);
			}
		}
	}

	_snow_arr_push(&_snow.desc_stack, &desc);

	_snow.current_desc =
		(struct _snow_desc *)_snow_arr_top(&_snow.desc_stack);

	if (desc.enabled && _snow.opts[_SNOW_OPT_LIST].boolval) {
		char *spaces = _snow_spaces(_snow.desc_stack.length - 1);
		_snow_print("%s%s\n", spaces, _snow.current_desc->full_name);
		return;
	}
}

__attribute__((unused))
static void _snow_desc_end(void) {
	if (_snow.current_desc->printed && !_snow.opts[_SNOW_OPT_LIST].boolval)
		_snow_print_desc_end();

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
#define _snow_case_begin(casename) \
	do { \
		if (_snow.opts[_SNOW_OPT_LIST].boolval) break; \
		if (!_snow.current_desc->enabled) break; \
		if (!_snow.current_desc->printed) _snow_print_desc_begin(); \
		_snow.in_case = 1; \
		_snow.current_case.success = 0; \
		_snow.current_case.name = casename; \
		_snow.current_case.start_time = _snow_now(); \
		_snow_arr_reset(&_snow.current_case.defers); \
		_snow_print_case_begin(); \
		_snow.current_desc->num_tests += 1; \
		if (_snow.current_desc->has_before_jmp) { \
			if (setjmp(_snow.current_case.before_jmp_ret) == 0) { \
				_snow.in_before_each = 1; \
				longjmp(_snow.current_desc->before_jmp, 1); \
			} \
			_snow.in_before_each = 0; \
		} \
		/* Set jump point which _snow_case_end */ \
		/* (and each defer) will jump back to */ \
		if (setjmp(_snow.current_case.done_jmp_ret) == 1) { \
			while (_snow.current_case.defers.length > 0) { \
				if (setjmp(_snow.current_case.defer_jmp_ret) == 0) { \
					jmp_buf *jmp = (jmp_buf *)_snow_arr_pop(&_snow.current_case.defers); \
					longjmp(*jmp, 1); \
				} \
			} \
			/* Run after_each */ \
			if (_snow.current_desc->has_after_jmp) { \
				if (setjmp(_snow.current_case.after_jmp_ret) == 0) { \
					_snow.in_after_each = 1; \
					longjmp(_snow.current_desc->after_jmp, 1); \
				} \
				_snow.in_after_each = 0; \
			} \
			/* Either re-run or just go back */ \
			int should_rerun = _snow.opts[_SNOW_OPT_RERUN_FAILED].boolval && \
				!_snow.rerunning_case && !_snow.current_case.success; \
			if (should_rerun) { \
				/* Run before_each again */ \
				if (_snow.current_desc->has_before_jmp) { \
					if (setjmp(_snow.current_case.before_jmp_ret) == 0) { \
						_snow.in_before_each = 1; \
						longjmp(_snow.current_desc->before_jmp, 1); \
					} \
					_snow.in_before_each = 0; \
				} \
				/* Actually re-run */ \
				_snow.rerunning_case = 1; \
				longjmp(_snow.current_case.rerun, 1); \
			} else { \
				_snow.rerunning_case = 0; \
				_snow.in_case = 0; \
			} \
		} \
	} while (0)

/*
 * Called after a test case block is done.
 */
__attribute__((unused))
static void _snow_case_end(int success) {
	if (!_snow.in_case)
		return;

	if (!_snow.rerunning_case) {
		_snow.current_case.success = success;
		if (success) {
			_snow.current_desc->num_success += 1;
			_snow_print_case_success();
		} else {
			_snow.exit_code = EXIT_FAILURE;
		}
	}

	longjmp(_snow.current_case.done_jmp_ret, 1);
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
static void _snow_case_defer_jmp(void) {
	longjmp(_snow.current_case.defer_jmp_ret, 1);
}

/*
 * Called when a before_each is done.
 * Will jump back to _snow_case_begin.
 */
__attribute__((unused))
static void _snow_before_each_end(void) {
	longjmp(_snow.current_case.before_jmp_ret, 1);
}

/*
 * Called when a after_each is done.
 * Will jump back to _snow_case_begin.
 */
__attribute__((unused))
static void _snow_after_each_end(void) {
	longjmp(_snow.current_case.after_jmp_ret, 1);
}

/*
 * Usage
 */
__attribute__((unused))
static void _snow_usage(char *argv0) {
	_snow_print("Usage: %s [options]            Run all tests.\n", argv0);
	_snow_print("       %s [options] <test>...  Run specific tests.\n", argv0);
	_snow_print("       %s -v|--version         Print version and exit.\n", argv0);
	_snow_print("       %s -h|--help            Display this help text and exit.\n", argv0);
	_snow_print("       %s -l|--list            Display a list of all the tests.\n", argv0);
	_snow_print(
		"\n"
		"Arguments:\n"
		"    --color|-c:     Enable colors.\n"
		"                    Default: on when output is a TTY.\n"
		"    --no-color:     Force disable --color.\n"
		"\n"
		"    --quiet|-q:     Suppress most messages, only test failures and a summary\n"
		"                    error count is shown.\n"
		"                    Default: off.\n"
		"    --no-quiet:     Force disable --quiet.\n"
		"\n"
		"    --log <file>:   Log output to a file, rather than stdout.\n"
		"\n"
		"    --timer|-t:     Display the time taken for by each test after\n"
		"                    it is completed.\n"
		"                    Default: on.\n"
		"    --no-timer:     Force disable --timer.\n"
		"\n"
		"    --maybes|-m:    Print out messages when begining a test as well\n"
		"                    as when it is completed.\n"
		"                    Default: on when the output is a TTY.\n"
		"    --no-maybes:    Force disable --maybes.\n"
		"\n"
		"    --cr:           Print a carriage return (\\r) rather than a newline\n"
		"                    after each --maybes message. This means that the fail or\n"
		"                    success message will appear on the same line.\n"
		"                    Default: on when the output is a TTY.\n"
		"    --no-cr:        Force disable --cr.\n"
		"\n"
		"    --rerun-failed: Re-run commands when they fail, after calling the\n"
		"                    'snow_break' function. Used by --gdb.\n"
		"                    Default: off.\n"
		"\n"
		"    --gdb, -g:      Run the test suite on GDB, and break and re-run\n"
		"                    test cases which fail.\n"
		"                    Default: off.\n");
    char *default_args[] = { "snow", SNOW_DEFAULT_ARGS };
    if (sizeof(default_args) > sizeof(char *) * 1) {
        _snow_print("\nCompiled with default arguments:");
        for (int i = 1; i < sizeof(default_args)/sizeof(char *); ++i) {
            _snow_print(" %s", default_args[i]);
        }
        _snow_print("\n");
    }
}

/*
 * Parse a single argument
 */
__attribute__((unused))
static int _snow_parse_args(char **args, int num) {
	int opts_done = 0;
	for (int i = 1; i < num; ++i) {
		char *arg = args[i];
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
		for (int j = 0; j < _SNOW_OPT_LAST; ++j) {
			struct _snow_opt *opt = _snow.opts + j;
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

				if (i + 1 >= num) {
					fprintf(stderr, "%s: Argument expected.", arg);
					return EXIT_FAILURE;
				}

				opt->strval = args[++i];
			}

			break;
		}

		if (!is_match) {
			fprintf(stderr, "Unknown option: %s\n", arg);
			return EXIT_FAILURE;
		}
	}

	return 0;
}

/*
 * The main function, which runs all top-level describes
 * and cleans up.
 */

__attribute__((unused))
static int snow_main_function(int argc, char **argv) {

	// There might be no tests, so we should init _snow here too
	if (!_snow_inited)
		_snow_init();

	/*
	 * Parse arguments
	 */
	int res;
	char *default_args[] = { "snow", SNOW_DEFAULT_ARGS };
	res = _snow_parse_args(default_args, sizeof(default_args)/sizeof(char *));
	if (res != 0)
		return res;

	res = _snow_parse_args(argv, argc);
	if (res != 0)
		return res;

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
			_snow.exit_code = EXIT_FAILURE;
			goto cleanup;
		} else {
			_snow.print.file_opened = 1;
		}
	}

	// --help and --version
	if (_snow.opts[_SNOW_OPT_HELP].boolval) {
		_snow_usage(argv[0]);
		goto cleanup;
	} else if (_snow.opts[_SNOW_OPT_VERSION].boolval) {
		_snow_print("Snow %s\n", SNOW_VERSION);
		goto cleanup;
	}

	// Set context-dependent defaults
	// (the context-independent defaults were set in the snow_main macro)
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
	if (getenv("NO_COLOR") != NULL) {
		_snow_opt_default(_SNOW_OPT_COLOR, 0);
	}

	// Other defaults
	_snow_opt_default(_SNOW_OPT_LIST, 0);
	_snow_opt_default(_SNOW_OPT_QUIET, 0);
	_snow_opt_default(_SNOW_OPT_TIMER, 1);
	_snow_opt_default(_SNOW_OPT_RERUN_FAILED, 0);
	_snow_opt_default(_SNOW_OPT_GDB, 0);

	// If --gdb was passed, re-run under GDB
	if (_snow.opts[_SNOW_OPT_GDB].boolval) {
#if SNOW_USE_FORK == 0
		fprintf(stderr, "Can't run GDB, because SNOW_USE_FORK is 0.");
		_snow.exit_code = EXIT_FAILURE;
		goto cleanup;
#else
		// Create temporary file
		char tmp_s[] = "/tmp/snow.XXXXXX";
		char tmp[sizeof(tmp_s)];
		strcpy(tmp, tmp_s);
		int tmpfd = mkstemp(tmp);
		if (tmpfd < 0) {
			perror("mkstemp");
			_snow.exit_code = EXIT_FAILURE;
			goto cleanup;
		}

		// Fill temporary file
		char ex[] =
			"break snow_break\n"
			"commands\n"
			"step\n"
			"end\n"
			"break snow_rerun_failed\n"
			"commands\n"
			"echo \\n*** Assertion failed while re-running. Stack trace:\\n\n"
			"bt\n"
			"end\n";
		if (write(tmpfd, ex, strlen(ex)) < 0) {
			perror("write");
			_snow.exit_code = EXIT_FAILURE;
			goto cleanup;
		}

		// Static arguments
		char *stdargv[] = {
			"gdb",
			"-x", tmp,
			"--args", argv[0], "--rerun-failed",
		};
		size_t stdargc = sizeof(stdargv) / sizeof(*stdargv);

		// Dynamic arguments
		char **args = malloc(sizeof(*args) * (stdargc + argc) + 1);
		size_t idx = 0;
		for (int i = 0; i < stdargc; ++i) {
			args[idx++] = stdargv[i];
		}
		for (int i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "--gdb") != 0 && strcmp(argv[i], "-g") != 0) {
				args[idx++] = argv[i];
			}
		}
		args[idx++] = NULL;

		// GDB handles SIGINT
		signal(SIGINT, SIG_IGN);

		// Fork
		pid_t child = fork();
		if (child < 0) {
			perror("fork");
			_snow.exit_code = EXIT_FAILURE;
			goto cleanup;
		}

		// Child
		if (child == 0) {
			signal(SIGINT, SIG_DFL);
			if (execvp("gdb", args) < 0) {
				perror("gdb");
			} else {
				fprintf(stderr,
					"execvp returned with no error, this should never happen.\n");
			}
			exit(EXIT_FAILURE);

		// Parent
		} else {
			int status;
			if (waitpid(child, &status, 0) < 0) {
				perror("waitpid");
				_snow.exit_code = EXIT_FAILURE;
				goto cleanup;
			}

			if (unlink(tmp) < 0) {
				perror(tmp);
				_snow.exit_code = EXIT_FAILURE;
				goto cleanup;
			}

			_snow.exit_code = WEXITSTATUS(status);
			goto cleanup;
		}
#endif
	}

	/*
	 * Run descs
	 */

	double total_start_time = _snow_now();
	int total_num_tests = 0;
	int total_num_success = 0;
	int total_descs_ran = 0;

	for (size_t i = 0; i < _snow.desc_funcs.length; ++i) {
		struct _snow_desc_func *df = _snow_arr_get(&_snow.desc_funcs, i);
		_snow_desc_begin(df->name);
		df->func();
		total_num_tests += _snow.current_desc->num_tests;
		total_num_success += _snow.current_desc->num_success;
		total_descs_ran += !!_snow.current_desc->printed;
		_snow_desc_end();
	}

	if (!_snow.opts[_SNOW_OPT_LIST].boolval) {
		int should_print_total =
			_snow.opts[_SNOW_OPT_QUIET].boolval ||
			total_descs_ran > 1;

		if (!_snow.opts[_SNOW_OPT_QUIET].boolval)
			_snow_print("\n");

		if (should_print_total) {
			if (_snow.opts[_SNOW_OPT_COLOR].boolval) {
				_snow_print(
						SNOW_COLOR_BOLD "Total: Passed %i/%i tests." SNOW_COLOR_RESET,
						total_num_success, total_num_tests);
			} else {
				_snow_print("Total: Passed %i/%i tests.",
						total_num_success, total_num_tests);
			}

			if (_snow.opts[_SNOW_OPT_TIMER].boolval) {
				_snow_print(" ");
				_snow_print_timer(total_start_time);
			}
			_snow_print("\n");

			if (!_snow.opts[_SNOW_OPT_QUIET].boolval)
				_snow_print("\n");
		}
	}

	/*
	 * Cleanup
	 */

cleanup:
	_snow_arr_reset(&_snow.desc_funcs);
	_snow_arr_reset(&_snow.desc_stack);
	_snow_arr_reset(&_snow.desc_patterns);
	_snow_arr_reset(&_snow.current_case.defers);
	_snow_arr_reset(&_snow.bufs.spaces);
	if (_snow.print.file_opened)
		fclose(_snow.print.file);

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
	__attribute__((optnone)) \
	static void snow_test_##name()

#define subdesc(name) \
	_snow_desc_begin(#name); \
	for (int _snow_desc_done = 0; _snow_desc_done == 0; \
			(_snow_desc_done = 1, _snow_desc_end()))

#define it(name) \
	_snow_case_begin(name); \
	if (_snow.opts[_SNOW_OPT_RERUN_FAILED].boolval) { \
		if (setjmp(_snow.current_case.rerun) == 1) { \
			snow_break(); \
		} \
	} \
	for (; _snow.in_case; _snow_case_end(1))
#define test it

#define defer(...) \
	do { \
		jmp_buf _snow_jmp; \
		if (setjmp(_snow_jmp) == 0) { \
			_snow_case_defer_push(_snow_jmp); \
		} else { \
			__VA_ARGS__; \
			_snow_case_defer_jmp(); \
		} \
	} while (0)

#define before_each() \
	_snow.current_desc->has_before_jmp = 1; \
	setjmp(_snow.current_desc->before_jmp); \
	for ( \
			int _snow_before_each_done = 0; \
			_snow_before_each_done == 0 && _snow.in_before_each; \
			(_snow_before_each_done = 1, _snow_before_each_end()))

#define after_each() \
	_snow.current_desc->has_after_jmp = 1; \
	setjmp(_snow.current_desc->after_jmp); \
	for ( \
			int _snow_after_each_done = 0; \
			_snow_after_each_done == 0 && _snow.in_after_each; \
			(_snow_after_each_done = 1, _snow_after_each_end()))

#define fail(...) \
	do { \
		snow_fail_update(); \
		snow_fail(__VA_ARGS__); \
	} while (0)

#define snow_main_decls \
	void snow_break() {} \
	void snow_rerun_failed() {} \
	struct _snow _snow; \
	int _snow_inited = 0

#define snow_main() \
	snow_main_decls; \
	int main(int argc, char **argv) { \
		return snow_main_function(argc, argv); \
	} \
	int _snow_unused_variable_for_semicolon

/*
 * Assert
 */

#define assert(x, expl...) \
	do { \
		snow_fail_update(); \
		const char *_snow_explanation = "" expl; \
		if (!(x)) \
			_snow_fail_expl(_snow_explanation, "Assertion failed: %s", #x); \
	} while (0)

/*
 * Various assert functions
 */

#define _snow_define_assertfunc(name, type, pattern) \
	__attribute__((unused)) \
	static int _snow_assert_##name( \
			int invert, const char *explanation, \
			const type a, const char *astr, const type b, const char *bstr) { \
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
_snow_define_assertfunc(dbl, long double, "%Lg")
_snow_define_assertfunc(ptr, void *, "%p")

__attribute__((unused))
static int _snow_assert_str(
		int invert, const char *explanation,
		const char *a, const char *astr, const char *b, const char *bstr) {
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

__attribute__((unused))
static int _snow_assert_buf(
		int invert, const char *explanation,
		const void *a, const char *astr, const void *b, const char *bstr, size_t size)
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

__attribute__((unused))
static int _snow_assert_fake(int invert, ...) {
	(void)invert;
	return -1;
}

// In mingw and on ARM, size_t is compatible with unsigned int, and
// ssize_t is compatible with int
#if(__SIZEOF_SIZE_T__ == __SIZEOF_INT__)
#define _snow_generic_assert(x) \
	_Generic((x), \
		float: _snow_assert_dbl, \
		double: _snow_assert_dbl, \
		long double: _snow_assert_dbl, \
		void *: _snow_assert_ptr, \
		char *: _snow_assert_str, \
		int: _snow_assert_int, \
		long long: _snow_assert_int, \
		unsigned int: _snow_assert_uint, \
		unsigned long long: _snow_assert_uint, \
		default: _snow_assert_fake)
#else
#define _snow_generic_assert(x) \
	_Generic((x), \
		float: _snow_assert_dbl, \
		double: _snow_assert_dbl, \
		long double: _snow_assert_dbl, \
		void *: _snow_assert_ptr, \
		char *: _snow_assert_str, \
		int: _snow_assert_int, \
		long long: _snow_assert_int, \
		ssize_t: _snow_assert_int, \
		unsigned int: _snow_assert_uint, \
		unsigned long long: _snow_assert_uint, \
		size_t: _snow_assert_uint, \
		default: _snow_assert_fake)
#endif

/*
 * Explicit asserteq macros
 */

#define asserteq_dbl(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_dbl( \
			0, "" expl, (a), #a, (b), #b); \
	} while (0)
#define asserteq_ptr(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_ptr( \
			0, "" expl, (a), #a, (b), #b); \
	} while (0)
#define asserteq_str(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_str( \
			0, "" expl, (a), #a, (b), #b); \
	} while (0)
#define asserteq_int(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_int( \
			0, "" expl, (a), #a, (b), #b); \
	} while (0)
#define asserteq_uint(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_uint( \
			0, "" expl, (a), #a, (b), #b); \
	} while (0)
#define asserteq_buf(a, b, size, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_buf( \
			0, "" expl, (a), #a, (b), #b, size); \
	} while (0)
#define asserteq_any(a, b, expl...) \
	do { \
		snow_fail_update(); \
		const char *_snow_explanation = "" expl; \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
		_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
		_Pragma("GCC diagnostic ignored \"-Wnull-pointer-arithmetic\"") \
		typeof ((a)+0) _a = a; \
		typeof ((b)+0) _b = b; \
		_Pragma("GCC diagnostic pop") \
		if (sizeof(_a) != sizeof(_b)) { /* NOLINT */ \
			_snow_fail_expl(_snow_explanation, \
				"Expected %s to equal %s, but their lengths don't match", \
				#a, #b); \
		} else { \
			_snow_assert_buf( \
				0, _snow_explanation, &_a, #a, &_b, #b, sizeof(_a)); \
		} \
	} while (0)

/*
 * Explicit assertneq macros
 */

#define assertneq_dbl(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_dbl( \
			1, "" expl, (a), #a, (b), #b); \
	} while (0)
#define assertneq_ptr(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_ptr( \
			1, "" expl, (a), #a, (b), #b); \
	} while (0)
#define assertneq_str(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_str( \
			1, "" expl, (a), #a, (b), #b); \
	} while (0)
#define assertneq_int(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_int( \
			1, "" expl, (a), #a, (b), #b); \
	} while (0)
#define assertneq_uint(a, b, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_uint( \
			2, "" expl, (a), #a, (b), #b); \
	} while (0)
#define assertneq_buf(a, b, size, expl...) \
	do { \
		snow_fail_update(); \
		_snow_assert_buf( \
			1, "" expl, (a), #a, (b), #b, (size)); \
	} while (0)
#define assertneq_any(a, b, expl...) \
	do { \
		snow_fail_update(); \
		const char *_snow_explanation = "" expl; \
		_Pragma("GCC diagnostic push") \
		_Pragma("GCC diagnostic ignored \"-Wpragmas\"") \
		_Pragma("GCC diagnostic ignored \"-Wpointer-arith\"") \
		_Pragma("GCC diagnostic ignored \"-Wnull-pointer-arithmetic\"") \
		typeof ((a)+0) _a = a; \
		typeof ((b)+0) _b = b; \
		_Pragma("GCC diagnostic pop") \
		if (sizeof(_a) != sizeof(_b)) { /* NOLINT */ \
			break; \
		} else { \
			_snow_assert_buf( \
				1, _snow_explanation, &_a, #a, &_b, #b, sizeof(_a)); \
		} \
	} while (0)

/*
 * Automatic asserteq
 */

#define asserteq(a, b, expl...) \
	do { \
		snow_fail_update(); \
		const char *_snow_explanation = "" expl; \
		int _snow_ret = _snow_generic_assert(b)( \
			0, _snow_explanation, (a), #a, (b), #b); \
		if (_snow_ret < 0) { \
			asserteq_any(a, b, expl); \
		} \
	} while (0)

/*
 * Automatic assertneq
 */

#define assertneq(a, b, expl...) \
	do { \
		snow_fail_update(); \
		const char *_snow_explanation = "" expl; \
		int _snow_ret = _snow_generic_assert(b)( \
			1, _snow_explanation, (a), #a, (b), #b); \
		if (_snow_ret < 0) { \
			assertneq_any(a, b, expl); \
		} \
} while (0)

#endif // SNOW_ENABLED

#endif // SNOW_H
