#include <stdio.h>
#include <snow/snow.h>

describe(defer_basic) {
	test("a") {
		defer printf("defer 1\n");
		defer printf("defer 2\n");
		defer printf("defer 3\n");
	}

	test("b") {
		defer printf("defer 1\n");
		defer printf("defer 2\n");
		defer printf("defer 3\n");
	}
}

describe(defer_plus_around) {
	before_each {
		printf("Before Each\n");
	}

	after_each {
		printf("After Each\n");
	}

	test("a") {
		defer printf("defer 1\n");
		defer printf("defer 2\n");
		defer printf("defer 3\n");
	}
}

snow_main();
