#include "snow.h"

#ifdef SNOW_ENABLED
describe(vector) {

	subdesc(vector_set) {
		it("hello there") {
			printf("hello there\n");
			defer(printf("hello\n"));
			defer(printf("world\n"));
		}
	}

	subdesc(vector_get) {
	}
}

describe(vaftor) {

	subdesc(vaftor_set) {
	}

	subdesc(vaftor_get) {
	}
}

snow_main()
#endif
