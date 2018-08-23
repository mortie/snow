#include "snow.h"

#ifdef SNOW_ENABLED
describe(vector) {
	before_each() {
		//printf("\nI am first before\n");
	}
	after_each() {
		//printf("I am first after\n");
	}
	it("hello there") {
		//printf("I am test, hello there\n");
		//defer(printf("hello 1\n"));
		//defer(printf("world 2\n"));
	}
	it("no u") {
		//defer(printf("I am first defer\n"));
		//printf("Hello, I am in 'no u' because in_case is %i\n", _snow.in_case);
		//defer(printf("I am second defer\n"));
	}

	subdesc(something) {
		before_each() {
			//printf("I am second before\n");
		}
		it("something in something") {
			//printf("hello, am in something in something\n");
		}
	}
}

snow_main()
#endif
