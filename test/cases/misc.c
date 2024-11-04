#include <snow/snow.h>

static inline int asserteq_call(int a, int b)
{
	asserteq(a, b);
	return a + b;
}

static inline int fail_call()
{
	fail("It has gone wrong :(");
	return 0;
}

describe(fail_in_call) {
	test("asserteq pass") { asserteq(asserteq_call(1, 1), 2); }
	test("asserteq fail outer") { asserteq(asserteq_call(1, 1), 3); }
	test("asserteq fail inner") { asserteq(asserteq_call(1, 2), 3); }
	test("fail inner") { asserteq(fail_call(), 0); }
}

snow_main();
