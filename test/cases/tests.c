#include <snow/snow.h>

// Create a fake gettimeofday which just increments with 1 second
// every time it's called.
static int currtime = 0;
#define gettimeofday(t, n) \
	do { \
		(t)->tv_sec = currtime++; \
		(t)->tv_usec = 0; \
	} while (0)

describe(a, {
	test("success", { assert(1); });
	test("failure", { assert(0); });
});

describe(b, {
	test("success", { assert(1); });
	test("failure", { assert(0); });
});

describe(c, {
	test("success", { assert(1); });
	test("success", { assert(1); });
});

describe(d, {
	test("failure", { assert(0); });
	test("success", { assert(1); });
});

snow_main();
