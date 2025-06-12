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

describe(f) {
	before_each() {
		fail("F BEFORE");
	}
	after_each() {
		puts("F AFTER");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }

	subdesc(ff) {
		before_each() {
			puts("FF BEFORE");
		}
		after_each() {
			puts("FF AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(fff) {
		before_each() {
			fail("FFF BEFORE");
		}
		after_each() {
			puts("FFF AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(ffff) {
		before_each() {
			puts("FFFF BEFORE");
		}
		after_each() {
			fail("FFFF AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(fffff) {
		before_each() {
			fail("FFFFF BEFORE");
		}
		after_each() {
			fail("FFFFF AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}

describe(g) {
	before_each() {
		puts("G BEFORE");
	}
	after_each() {
		fail("G AFTER");
	}
	test("success") { assert(1); }
	test("failure") { assert(0); }

	subdesc(gg) {
		before_each() {
			puts("GG BEFORE");
		}
		after_each() {
			puts("GG AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(ggg) {
		before_each() {
			fail("GGG BEFORE");
		}
		after_each() {
			puts("GGG AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(gggg) {
		before_each() {
			puts("GGGG BEFORE");
		}
		after_each() {
			fail("GGGG AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}

	subdesc(ggggg) {
		before_each() {
			fail("GGGGG BEFORE");
		}
		after_each() {
			fail("GGGGG AFTER");
		}
		test("success") { assert(1); }
		test("failure") { assert(0); }
	}
}

snow_main();
