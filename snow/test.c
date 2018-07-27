#include "snow.h"
#include <unistd.h>

describe(vector) {

	it("breaks the rules of math (to demonstrate failed tests)") {
		assert(1 == 2);
	}

	it("inits vectors correctly") {
	}

	it("allocates vectors based on elem_size") {
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
