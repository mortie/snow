[![Build Status](https://travis-ci.org/mortie/snow.svg?branch=master)](https://travis-ci.org/mortie/snow)

# Snow

Snow is a header-only unit testing library for C. Just include the file
[snow/snow.h](https://github.com/mortie/snow/blob/v2.3.0/snow/snow.h).

IRC channel: [#snow](http://webchat.freenode.net?channels=snow) on Freenode.
If you have any questions, or just want to chat, just ping me (@mort) :)

![Screenshot](https://raw.githubusercontent.com/mortie/snow/master/img/screenshot.png)

## Snow 2

Snow 2 is a complete rewrite of Snow. Here are the highlights:

* Blocks have moved from inside of macro arguments (i.e `describe(foo, { ... })`)
  to outside of macro arguments (i.e `describe(foo) { ... }`). This applies to
  `describe`, `subdesc`, `it`/`test`, `before_each`, and `after_each`.
	* This means that it's possible to show line numbers, that compiler error
	  messages are nicer, and syntax highlighters and auto indenters should be
	  more happy.
* `asserteq` and `assertneq` works slightly differently, but most code which
  worked before should continue to work.
* All assertion macros have gotten an extra, optional argument, which is an
  explanation of what the assertion means. For example, you can now write
  `asserteq(foo, bar, "Some explanation")`.
* You can select what tests to run with glob-style matches, not just filter
  based on the name of the top-level describe.

## About

Some miscellaneous points:

* Snow uses some GNU extensions, so it might not work with all
  ISO C compatible compilers. It's confirmed to work with at least GCC and
  Clang. It should even work on GCC and Clang versions too old to support C11
  (or even C99), but the convenience `asserteq` and `assertneq` macros require
  C11.
* I really recommend running the test executable with
  [valgrind](http://valgrind.org/). That will help you find memory issues such
  as memory leaks, out of bounds array reads/writes, etc.
* Windows is supported through MinGW or cygwin, with the caveat that it assumes
  your terminal supports UTF-8. CMD.exe and Powershell will print mangled ✓ and ✕
  characters. (Git Bash and Cygwin's terminal should be fine though)
	* Windows also generally doesn't have the `<fnmatch.h>` header.
	  Snow defaults to compile without fnmatch under MinGW
	  (and instead uses plain strcmp). You can control this with
	  `-DSNOW_USE_FNMATCH=1` or `-DSNOW_USE_FNMATCH=0`.
	  [Gnulib](https://www.gnu.org/software/gnulib/manual/gnulib.html)
	  implements fnmatch, and supports Windows under Cygwin.

## Usage

When creating the main function using the `snow_main` macro, your executable
will take these arguments. The **--no-** prefixed arguments will disable the
relevant function:

* **--version**, **-v**: Show the current version and exit.
* **--help**, **-h**: Show usage and exit.
* **--list**, **-l**: List available tests and exit.
* **--color**, **-c**, or **--no-color**: Enable the use of color.
  Default: on when output is a TTY, off otherwise.
* **--quiet**, **-q**, or **--no-quiet**: Suppress most messages, only test faulures
  and the 'Total: Passed X/Y tests' line will still print.
  Default: off.
* **--log \<file\>**: Output to a log file instead of stdout.
* **--timer**, **-t**, or **--no-timer**: Print the number of miliseconds CPU time
  spent on each test alongside its success message.
  Default: on.
* **--maybes**, **-m**, or **--no-maybes**: Print out messages when beginning a test
  rather than just when it completed.
  Default: on when output is a TTY, off otherwise.
* **--cr**, or **--no-cr**: Print a `\r` after maybe messages instead of `\n`. This
  will override them with successes or failures as they are printed out.
  Default: on when output is a TTY, off otherwise.

## Example

Here's a simple example which tests a couple of filesystem functions, and has a
subdescription for testing fread-related stuff.

* Compile: `gcc -Isnow -DSNOW_ENABLED -g -o test example.c`
	* `-Isnow`: add `snow` to our include path, to make `#include <snow/snow.h>`
	  work. (That assumes `snow/snow/snow.h` exists, like if you clone this repo.)
	* `-DSNOW_ENABLED`: Enable snow (otherwise `describe(...)` would be
	  compiled down to nothing).
	* `-g`: Add debug symbols for valgrind.
* Run: `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./test`

``` C
#include <stdio.h>
#include <snow/snow.h>

describe(files) {
	it("opens files") {
		FILE *f = fopen("test", "r");
		assertneq(f, NULL);
		defer(fclose(f));
	}

	it("writes to files") {
		FILE *f = fopen("testfile", "w");
		assertneq(f, NULL);
		defer(remove("testfile"));
		defer(fclose(f));

		char str[] = "hello there";
		asserteq(fwrite(str, 1, sizeof(str), f), sizeof(str));
	}

	subdesc(fread) {
		it("reads 10 bytes") {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[10];
			asserteq(fread(buf, 1, 10, f), 10);
		}

		it("reads 20 bytes") {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[20];
			asserteq(fread(buf, 1, 20, f), 20);
		}
	}
}

snow_main();
```

## Compile options

* **SNOW\_ENABLED**: Define to enable Snow.
* **SNOW\_USE\_FNMATCH**: Set to 0 to not use fnmatch for test name
  matching, and instead just compare literal strings. (Useful for systems
  without fnmatch)
* **SNOW\_COLOR\_SUCCESS**: The escape sequence before printing success.
* **SNOW\_COLOR\_FAIL**: The escape sequence before printing failure.
* **SNOW\_COLOR\_MAYBE**: The escape sequence before printing maybes.
* **SNOW\_COLOR\_DESC**: The escape sequence before printing the test
  description.
* **SNOW\_COLOR\_BOLD**: The escape sequence for bold text.
* **SNOW\_COLOR\_RESET**: The escape sequence to reset formatting.

## Structure Macros

### describe(testname) \<block>

A top-level description of a component, which can contain `subdesc`s and `it`s.
A `describe(testname, block)` will define a function `void test_##testname()`,
which the main function created by `snow_main` will call automatically.

### subdesc(testname) \<block>

A description of a sub-component, which can contain nested `subdesc`s and
`it`s. It's similar to `describe`, but doesn't define a function.

### it(description) \<block>

A particular test case. It can contain asserts and `defer`s, as well as just
regular code. A failing assert (or direct call to `fail(...)`) will mark the
test as failed, but if it completes normally, it's marked as successful.

### defer(expr)

`defer` is used for tearing down, and is inspired by Go's [defer
statement](https://gobyexample.com/defer).

Once the test case completes, each deferred expression will be executed, in the
reverse order of their definitions (i.e `defer(printf("World"));
defer(printf("Hello "));` will print "Hello World"). If the test case fails,
only deferred expressions defined before the point of failure will be executed.

### before\_each() \<block>

Code to run before each test case.

### after\_each() \<block>

Code to run after each test case.

### snow\_main()

This macro expands to a main function which handless stuff like parsing
arguments and freeing memory allocated by Snow. All described functions will
automatically be called by the main functions.

## Assert Macros

### fail(fmt, ...)

Just directly fail the test case. The arguments are a printf-style format,
optionally followed by arguments, just like `printf`.

### assert(x [, explanation])

Fail if the expression `x` returns 0. `explanation`  is an optional string
which will be printed if the assertion fails, and can be used to provide some
context.

### asserteq(a, b [, explanation])

Fail unless `a` equals `b`. If `b` is a string, `strcmp` will be used to check
for equality; otherwise, `==` will be used.

`asserteq` requires C11.
If you can't use C11, or want to explicitly state what type your arguments are
(say you want to compare strings by pointer instead of by content), you
can use the `asserteq_int`, `asserteq_ptr`, `asserteq_dbl`, and `asserteq_str`
macros instead of `asserteq`.

### assertneq(a, b [, explanation])

Fail if `a` equals `b`. If `b` is a string, `strcmp` will be used to check
for equality; otherwise, `==` will be used.

`assertneq` requires C11.
If you can't use C11, or want to explicitly state what type your arguments are
(say you want to compare strings by pointer instead of by content), you
can use the `assertneq_int`, `assertneq_ptr`, `assertneq_dbl`, and `asserteq_str`
macros instead of `assertneq`.

### asserteq\_buf(a, b, n [, explanation])

Fail unless the first `n` bytes of `a` and `b` are the same.

### assertneq\_buf(a, b, n [, explanation])

Fail if the first `n` bytes of `a` and `b` are the same.

### snow\_fail(fmt, ...), snow\_fail\_update()

`snow_fail_update` saves the current file/line, while `snow_fail` fails the
currently executing test case and prints the saved file/line from the last
`snow_fail_update`. This allows for implementing new checks to fail tests.
All assertion functions from Snow are implemented using `snow_fail` and
`snow_fail_update`.

## How to test

Exactly how to test your code might not be as obvious with C as it is for other
languages. I haven't yet used Snow for any big projects, but here's how I would
do it

### Testing a library's interface

When testing a library's interface, you can just create a `test` folder which
is completely decoupled from the library's source code, and just compile your
test code with a flag to link against your library.

### Testing a program or library internals

Testing anything that's not exposed as a library's public API is is possible
because unless `SNOW_ENABLED` is defined, `describe` will just be an empty
macro, and all uses of it will be removed by the preprocessor. Therfore, you
can include tests directly in your C source files, and regular builds won't
use snow, while builds with `-DSNOW_ENABLED` will be your test suite.

Since all test macros are compiled down to nothing, this will have no runtime
performance or binary size impact.

Note that since `snow.h` defines macros with pretty general names (`it`,
`describe`, `assert`), it's probably a good idea to put your tests at the
bottom of the source and only include `snow.h` right above the test code, to
avoid name conflicts.

The [exampleproject](https://github.com/mortie/snow/blob/master/exampleproject)
directory is an example of a program tested this way.
