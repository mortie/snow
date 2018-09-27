#include <snow/snow.h>

describe(a) {
	before_each() {
		puts("A BEFORE");
	}
	after_each() {
		puts("A AFTER");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }
}

describe(b) {
	test("success") { assert(1); }
	test("failure") { assert(0); }
	subdesc(bb) {
		before_each() {
			puts("BB BEFORE");
		}
		after_each() {
			puts("BB AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}

describe(c) {
	before_each() {
		puts("C BEFORE");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }
	subdesc(cc) {
		after_each() {
			puts("CC AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}

describe(d) {
	before_each() {
		puts("D BEFORE");
	}
	after_each() {
		puts("D AFTER");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }
	subdesc(dd) {
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}

describe(e) {
	before_each() {
		puts("E BEFORE");
	}
	after_each() {
		puts("E AFTER");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }
	subdesc(ee) {
		before_each() {
			puts("EE BEFORE");
		}
		after_each() {
			puts("EE AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}
snow_main();
