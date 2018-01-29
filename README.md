# Snow

Snow is a header-only unit testing library for C. The file
[snow/snow.h](https://github.com/mortie/snow/blob/master/snow/snow.h) should
be included from the main source file.

Look at [example.c](https://github.com/mortie/snow/blob/master/example.c) for an example.

![Screenshot](https://raw.githubusercontent.com/mortie/snow/master/img/screenshot.png)

## About

Some miscellaneous points:

* If your test suite consists of multiple files, the various `.c` files should
  be included from the main source file, because Snow uses some static
  globals. Each `.c` file should probably still include `snow.h` to help your
  editor out.
* The defer feature uses some GNU extensions, so Snow might not work with all
  ISO C compatible compilers. It's confirmed to work with at least GCC and
  Clang.
* Even though Snow is based on passing blocks to the C preprocessor macros,
  unguarded commas are no problem, because the `block` argument is actually
  implemented as `...` and expanded with `__VA_ARGS__`.

## Arguments

Since you're not supposed to make your own main function, and instead use the
`snow_main` macro, your binary will take these arguments:

* **--color**: Force the use of color, even when stdout is not a TTY or
  NO\_COLOR is set.
* **--no-color**: Force colors to be disabled, evern when stdout is a TTY.

## Example

Here's a simple example which tests a couple of filesystem functions, and has a
subdescription for testing fread-related stuff. It's very similar to the
example in [example.c](https://github.com/mortie/snow/blob/master/example.c).

	describe(files, {
		it("opens files", {
			FILE *f = fopen("test", "r");
			assertneq(f, NULL);
			defer(fclose(f));
		});

		it("writes to files", {
			FILE *f = fopen("testfile", "w");
			assertneq(f, NULL);
			defer(remove("testfile"));
			defer(fclose(f));

			char str[] = "hello there";
			asserteq(fwrite(str, 1, sizeof(str), f), sizeof(str));
		});

		subdesc(fread, {
			it("reads 10 bytes", {
				FILE *f = fopen("/dev/zero", "r");
				assertneq(f, NULL);
				defer(fclose(f));

				char buf[10];
				asserteq(fread(buf, 1, 10, f), 10);
			});

			it("reads 20 bytes", {
				FILE *f = fopen("/dev/zero", "r");
				assertneq(f, NULL);
				defer(fclose(f));

				char buf[20];
				asserteq(fread(buf, 1, 20, f), 20);
			});
		});
	});

	snow_main();

## Structure Macros

### describe(testname, block)

A top-level description of a component, which can contain `subdesc`s and `it`s.
A `describe(testname, block)` will define a function `void test_##testname()`,
which the main function created by `snow_main` will call automatically.

### subdesc(testname, block)

A description of a sub-component, which can contain nested `subdesc`s and
`it`s. It's similar to `describe`, but doesn't define a function.

### it(description, block)

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

### snow\_main()

This macro expands to a main function which handless stuff like parsing
arguments and freeing memory allocated by Snow. All described functions will
automatically be called by the main functions.

## Assert Macros

### fail(fmt, ...)

Just directly fail the test case. The arguments are a printf-style format,
optionally followed by arguments, just like `printf`.

### assert(x)

Fail if the expression `x` returns 0.

### asserteq(a, b)

Fail unless `a` equals `b`. If `b` is a string, `strcmp` will be used to check
for equality; otherwise, `==` will be used.

If you want to explicitly state what type your arguments are
(say you want to compare strings by pointer instead of by content), you
can use the `asserteq_int`, `asserteq_ptr`, `asserteq_dbl`, and `asserteq_str`
macros.

### assertneq(a, b)

Fail if `a` equals `b`. If `b` is a string, `strcmp` will be used to check
for equality; otherwise, `==` will be used.

If you want to explicitly state what type your arguments are
(say you want to compare strings by pointer instead of by content), you
can use the `assertneq_int`, `assertneq_ptr`, `assertneq_dbl`, and `asserteq_str`
macros.

### asserteq\_buf(a, b, n)

Fail unless the first `n` bytes of `a` and `b` are the same.

### assertneq\_buf(a, b, n)

Fail if the first `n` bytes of `a` and `b` are the same.
