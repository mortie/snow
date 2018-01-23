# Testfw

testfw is a testing framework for C, as a header-only library. The file
[test/test.h](https://github.com/mortie/testfw/blob/master/test/test.h) should
be included from the main source file.

Look at [example.c](https://github.com/mortie/testfw/blob/master/example.c) for an example.

![Screenshot](https://raw.githubusercontent.com/mortie/testfw/master/img/screenshot.png)

## About

Some miscellaneous points:

* If your test suite consists of multiple files, the various `.c` files should
  be included from the main source file, because testfw uses some static
  globals. Each `.c` file should probably still include `test.h` to help your
  editor out.
* Becauste testfw is based on passing blocks to C preprocessor macros, make
  sure to not include any unguarded commas (commas outside of parentheses). The
  only case where I know that's relevant is in variable declarations like
  `int a, b, c;` - just make sure to use `int a; int b; int c;` instead.

## Example

Here's a simple example which tests a couple of filesystem functions, and has a
subdescription for testing fread-related stuff. It's very similar to the
example in [example.c](https://github.com/mortie/testfw/blob/master/example.c).

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

			char *str = "hello there";
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

	int main() {
		test_files();
		done();
	}

## Structure Macros

### describe(testname, block)

A top-level description of a component, which can contain `subdesc`s and `it`s.
A `describe(testname, block)` will define a function `void test_##testname()`,
which the main function should call.

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

### done()

Must be called at the end of the main function. It will print the total count
of total and successful tests (if there's more than one top-level description),
and return with an appropriate exit code (0 if no tests failed, 1 if tests
failed).

## Assert Macros

### fail(fmt, ...)

Just directly fail the test case. The arguments are a printf-style format,
optionally followed by arguments, just like `printf`.

### assert(x)

Fail if the expression `x` returns 0.

### asserteq(a, b)

Fail unless `a == b`.

### assertneq(a, b)

Fail unless `a != b`.

### assertstreq(a, b)

Fail unless `strcmp(a, b) == 0`, 

### assertstrneq(a, b)

Fail unless `strcmp(a, b) != 0`.
