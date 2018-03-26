#include <snow/snow.h>

// Create a fake gettimeofday which just increments with 1 second
// every time it's called.
static int currtime = 0;
#define gettimeofday(t, n) \
	do { \
		(t)->tv_sec = currtime++; \
		(t)->tv_usec = 0; \
	} while (0)

describe(test, {
	test("success", { assert(1); });
	test("failure", { assert(0); });
});

snow_main();
