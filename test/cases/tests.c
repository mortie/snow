#include <snow/snow.h>

describe(a) {
	test("success") { assert(1); }
	test("failure") { assert(0); }
}

describe(b) {
	test("success") { assert(1); }
	test("failure") { assert(0); }
}

describe(c) {
	test("success") { assert(1); }
	test("success") { assert(1); }
}

describe(d) {
	test("failure") { assert(0); }
	test("success") { assert(1); }
}

snow_main();
