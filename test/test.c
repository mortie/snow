#define _DEFAULT_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static int bufeq(char *b1, char *b2, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		if (b1[i] != b2[i])
			return 0;

	return 1;
}

static int runcase(FILE *f, int *results, size_t count)
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

	if (resultidx != count)
		return -1;
	else
		return 0;
}

#define SUCCESS 1
#define FAILURE 0

#define EQ_FAILURE 0
#define EQ_SUCCESS 1
#define NEQ_SUCCESS 2
#define NEQ_FAILURE 3
#define TEST_WORKED 4

#include <snow/snow.h>

describe(asserts, {
	FILE *f = popen("./cases/asserts", "r");

	test("asserteq_int", {
		int results[4];
		asserteq(runcase(f, results, 4), 0);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	});

	test("asserteq_dbl", {
		int results[4];
		asserteq(runcase(f, results, 4), 0);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	});

	test("asserteq_ptr", {
		int results[5];
		asserteq(runcase(f, results, 5), 0);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
		asserteq(results[TEST_WORKED], SUCCESS);
	});

	test("asserteq_str", {
		int results[4];
		asserteq(runcase(f, results, 4), 0);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	});

	test("asserteq_buf", {
		int results[4];
		asserteq(runcase(f, results, 4), 0);

		asserteq(results[EQ_FAILURE], FAILURE);
		asserteq(results[EQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_SUCCESS], SUCCESS);
		asserteq(results[NEQ_FAILURE], FAILURE);
	});

	pclose(f);
});

snow_main();
