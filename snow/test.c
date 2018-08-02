#include "snow.h"
#include <unistd.h>

describe(vector) {

	it("breaks the rules of math (to demonstrate failed tests)") {
		assert(1 == 2, "sanity check");
	}

	it("inits vectors correctly") {
		char *foo = "hello";
		asserteq(foo, "hello");

		struct { int a; } a = { 10 };
		struct { long long b; } b = { 100 };
		asserteq(a, b);
	}

	it("allocates vectors based on elem_size") {
		uint8_t foo[] = "hello world how are you";
		uint8_t bar[] = "hello world how are you";
		asserteq_buf(foo, bar, sizeof(foo), "Sanity check");
	}

	subdesc(vector_set) {
		it("sets values inside of the allocated range") {
		}

		it("allocates space when setting values outside the allocated range") {
		}
	}

	subdesc(vector_get) {
		it("gets values inside the allocated range") {
		}

		it("returns NULL when trying to access values outside the allocated range") {
		}
	}
}

#ifdef SNOW_ENABLED
snow_main()
#endif
