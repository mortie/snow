#include <snow/snow.h>

describe(test) {
	test("success") { assert(1); }
	test("failure") { assert(0); }
}

snow_main();
