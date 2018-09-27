#define _DEFAULT_SOURCE

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#ifndef __MINGW32__
#include <sys/wait.h>
#endif

// This is relatively horrible and does no escaping,
// so make sure that 'str' doesn't contain a single quote, k?
// It's here because we want to use sh even on Windows under mingw.
static FILE *runcmd(char *str)
{
	char command[512];
	snprintf(command, sizeof(command) - 1, "sh -c '%s'", str);
	command[sizeof(command) - 1] = '\0';
	return popen(command, "r");
}

static int bufeq(char *b1, char *b2, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		if (b1[i] != b2[i])
			return 0;

	return 1;
}

#define SUCCESS 1
#define FAILURE 0

/*
 * Returns the amount of results. Fills the 'results' array with whether
 * a test failed or not.
 */
static int getResults(FILE *f, int *results, size_t count)
{
	char success[] = "✓";
	char failure[] = "✕";

	size_t resultidx = 0;

	int linedone = 0;
	char linestart[4];
	int linestartidx = 0;
	int c;
	while ((c = getc(f)) != EOF)
	{
		if ((char)c == '\n')
			linedone = 0;

		if (isspace(c))
			continue;

		if (!linedone)
		{
			linestart[linestartidx++] = (char)c;
			if (linestartidx == sizeof(linestart))
			{
				linedone = 1;
				linestartidx = 0;

				if (bufeq(linestart, success, sizeof(success) - 1))
					results[resultidx++] = 1;
				else if (bufeq(linestart, failure, sizeof(failure) - 1))
					results[resultidx++] = 0;

				if (resultidx == count)
					break;
			}
		}
	}

	return resultidx;
}

/*
 * Compare two files.
 * 1 means they're the same, 0 means they're different.
 */
static int compareFiles(FILE *f1, FILE *f2)
{
	int c1, c2;
	int cnt = 0;
	do
	{
		c1 = fgetc(f1);
		c2 = fgetc(f2);
		if (c1 != c2)
		{
			return 0;
		}
		cnt += 1;
	} while (c1 != EOF && c2 != EOF);

	return c1 == c2;
}

/*
 * Compare a command's output to a file.
 * 1 means they're the same, 0 means they're different.
 */
static int compareOutput(char *cmd, char *expected)
{
	FILE *f1 = runcmd(cmd);

	char path[256];
	path[sizeof(path) - 1] = '\0';
	snprintf(path, sizeof(path) - 1, "./expected/%s", expected);
	FILE *f2 = fopen(path, "r");

	int res = compareFiles(f1, f2);
	pclose(f1);
	fclose(f2);
	return res;
}

#define EQ_FAILURE 0
#define EQ_SUCCESS 1
#define NEQ_SUCCESS 2
#define NEQ_FAILURE 3
#define TEST_WORKED 4

#include <snow/snow.h>

#ifdef SNOW_ENABLED
describe(asserts) {
	FILE *f = runcmd("./cases/asserts");

	test("asserteq_int, assertneq_int") {
		int results[4];
		asserteq(getResults(f, results, 4), 4);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	}

	test("asserteq_dbl, assertneq_dbl") {
		int results[4];
		asserteq(getResults(f, results, 4), 4);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	}

	test("asserteq_ptr, assertneq_str") {
		int results[5];
		asserteq(getResults(f, results, 5), 5);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
		asserteq(results[TEST_WORKED], SUCCESS);
	}

	test("asserteq_str, assertneq_str") {
		int results[4];
		asserteq(getResults(f, results, 4), 4);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	}

	test("asserteq_buf, assertneq_buf") {
		int results[4];
		asserteq(getResults(f, results, 4), 4);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	}

	test("asserteq") {
		int results[12];
		asserteq(getResults(f, results, 12), 12);

		asserteq(results[0], SUCCESS);
		asserteq(results[1], FAILURE);

		asserteq(results[2], SUCCESS);
		asserteq(results[3], FAILURE);

		asserteq(results[4], SUCCESS);
		asserteq(results[5], FAILURE);

		asserteq(results[6], SUCCESS);
		asserteq(results[7], FAILURE);

		asserteq(results[8], SUCCESS);
		asserteq(results[9], FAILURE);

		asserteq(results[10], SUCCESS);
		asserteq(results[11], FAILURE);
	}

	test("assertneq") {
		int results[12];
		asserteq(getResults(f, results, 12), 12);

		asserteq(results[0], FAILURE);
		asserteq(results[1], SUCCESS);

		asserteq(results[2], FAILURE);
		asserteq(results[3], SUCCESS);

		asserteq(results[4], FAILURE);
		asserteq(results[5], SUCCESS);

		asserteq(results[6], FAILURE);
		asserteq(results[7], SUCCESS);

		asserteq(results[8], FAILURE);
		asserteq(results[9], SUCCESS);

		asserteq(results[10], FAILURE);
		asserteq(results[11], SUCCESS);
	}

	pclose(f);
}

describe(commandline) {
// When running with git bash, argv[0] will be an absolute path, so
// this test case would fail, because it assumes the -h option prints
// the actual path used to run the binary.
#ifndef __MINGW32__
	it("prints usage with -h and --help") {
		assert(compareOutput("./cases/commandline --help", "commandline-help"));
		assert(compareOutput("./cases/commandline -h", "commandline-help"));
	}
#endif

	it("prints version with -v and --version") {
		assert(compareOutput("./cases/commandline --version", "commandline-version"));
		assert(compareOutput("./cases/commandline -v", "commandline-version"));
	}

	it("prints only failure and a total with --quiet") {
		assert(compareOutput("./cases/commandline --quiet", "commandline-quiet"));
		assert(compareOutput("./cases/commandline -q", "commandline-quiet"));
	}

	it("prints times") {
		assert(compareOutput("./cases/commandline", "commandline-timer"));
		assert(compareOutput("./cases/commandline -t", "commandline-timer"));
		assert(compareOutput("./cases/commandline --timer", "commandline-timer"));
	}

	it("prints no times with --no-timer") {
		assert(compareOutput("./cases/commandline --no-timer", "commandline-no-timer"));
	}

	it("logs to the file specified with --log") {
		int res = compareOutput("./cases/commandline --log tmpfile", "commandline-log-stdout");
		defer(unlink("tmpfile"));
		assert(res);

		FILE *f1 = fopen("tmpfile", "r");
		assertneq(f1, NULL);
		defer(fclose(f1));

		FILE *f2 = fopen("./expected/commandline-log-output", "r");
		assertneq(f2, NULL);
		defer(fclose(f2));

		assert(compareFiles(f1, f2));
	}
}

describe(tests) {
	it("performs all the tests") {
		assert(compareOutput("./cases/tests", "tests-all"));
	}

	it("performs only a specific set of tests") {
		assert(compareOutput("./cases/tests a c", "tests-specific"));
	}

	it("performs a single test") {
		assert(compareOutput("./cases/tests a", "tests-single"));
	}

#ifndef __MINGW32__
	it("fails when asked to run a non-existant test suite") {
		asserteq(
			WEXITSTATUS(system("./cases/tests a b doesnt-exist >/dev/null")),
			EXIT_FAILURE);
	}
#endif
}

describe(around) {
	it("before_each and after_each should run before and after each test") {
		assert(compareOutput("./cases/around a", "around-before-after"));
	}

	it("subdesc use parent describe's before_each and after_each") {
		assert(compareOutput("./cases/around d", "around-subdesc-parent-before-after"));
	}

	it("before_each and after_each should only run in a subdesc clause") {
		assert(compareOutput("./cases/around b", "around-before-after-only-in-subdesc"));
	}

	it("after_each in parent should complement subdesc") {
		assert(compareOutput("./cases/around c", "around-after-complements-subdesc"));
	}

	it("before_each and after_each in subdesc should shadow the parent") {
		assert(compareOutput("./cases/around e", "around-subdesc-before-after-shadow"));
	}
}

snow_main();
#endif
