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
	before_each({
		puts("A BEFORE");
	});
	after_each({
		puts("A AFTER");
	});
	test("success", { assert(1); });
	test("failure", { assert(0); });
});

describe(b, {
	test("success", { assert(1); });
	test("failure", { assert(0); });
	subdesc(bb, {
		before_each({
			puts("BB BEFORE");
		});
		after_each({
			puts("BB AFTER");
		});
		test("success", { assert(1); });
		test("failure", { assert(0); });
	});
});

describe(c, {
	before_each({
		puts("C BEFORE");
	});
	test("success", { assert(1); });
	test("failure", { assert(0); });
	subdesc(cc, {
		after_each({
			puts("CC AFTER");
		});
		test("success", { assert(1); });
		test("failure", { assert(0); });
	});
});

describe(d, {
	before_each({
		puts("D BEFORE");
	});
	after_each({
		puts("D AFTER");
	});
	test("success", { assert(1); });
	test("failure", { assert(0); });
	subdesc(dd, {
		test("success", { assert(1); });
		test("failure", { assert(0); });
	});
});


snow_main();
