#include <snow/snow.h>

describe(asserteq_int) {
	test("eq failure") { asserteq_int(1, 2); }
	test("eq success") { asserteq_int(1, 1); }
	test("neq success") { assertneq_int(1, 2); }
	test("neq failure") { assertneq_int(1, 1); }
}

describe(asserteq_dbl) {
	test("eq failure") { asserteq_dbl(1.0, 2.0); }
	test("eq success") { asserteq_dbl(1.0, 1.0); }
	test("neq success") { assertneq_dbl(1.0, 2.0); }
	test("neq failure") { assertneq_dbl(1.0, 1.0); }
}

describe(asserteq_ptr) {
	int *ptr1 = calloc(1, sizeof(int));
	int *ptr2 = calloc(1, sizeof(int));

	test("eq failure") { asserteq_ptr(ptr1, ptr2); }
	test("eq success") { asserteq_ptr(ptr1, ptr1); }
	test("neq success") { assertneq_ptr(ptr1, ptr2); }
	test("neq failure") { assertneq_ptr(ptr1, ptr1); }

	test("worked") {
		assertneq(ptr1, NULL);
		assertneq(ptr2, NULL);
	}

	free(ptr1);
	free(ptr2);
}

describe(asserteq_str) {
	test("eq failure") { asserteq_str("hello", "there"); }
	test("eq success") { asserteq_str("hello", "hello"); }
	test("neq success") { assertneq_str("hello", "there"); }
	test("neq failure") { assertneq_str("hello", "hello"); }
}

describe(asserteq_buf) {
	int buf1[] = { 0, 2, 3, 1 };
	int buf2[] = { 2, 2, 1, 104 };
	size_t len = 4;

	test("eq failure") { asserteq_buf(buf1, buf2, len); }
	test("eq success") { asserteq_buf(buf1, buf1, len); }
	test("neq success") { assertneq_buf(buf1, buf2, len); }
	test("neq failure") { assertneq_buf(buf1, buf1, len); }
}

describe(asserteq) {
	test("eq success") { asserteq(1, 1); }
	test("eq failure") { asserteq(1, 2); }

	test("eq success") { asserteq(1.0, 1.0); }
	test("eq failure") { asserteq(1.0, 2.0); }

	int *ptr1 = calloc(1, sizeof(int));
	int *ptr2 = calloc(1, sizeof(int));
	test("eq success") { asserteq(ptr1, ptr1); }
	test("eq failure") { asserteq(ptr1, ptr2); }
	free(ptr1);
	free(ptr2);

	test("eq success") { asserteq("hello", "hello"); }
	test("eq failure") { asserteq("hello", "world"); }

	char str1[] = "hello";
	char str2[] = "world";
	test("eq success") { asserteq(str1, str1); }
	test("eq success") { asserteq(str1, str2); }

	void *vptr1 = NULL;
	void *vptr2 = (void *)0x10;
	test("eq success") { asserteq(vptr1, vptr1); }
	test("eq success") { asserteq(vptr1, vptr2); }
}

describe(assertneq) {
	test("neq failure") { assertneq(1, 1); }
	test("neq success") { assertneq(1, 2); }

	test("neq failure") { assertneq(1.0, 1.0); }
	test("neq success") { assertneq(1.0, 2.0); }

	int *ptr1 = calloc(1, sizeof(int));
	int *ptr2 = calloc(1, sizeof(int));
	test("neq failure") { assertneq(ptr1, ptr1); }
	test("neq success") { assertneq(ptr1, ptr2); }
	free(ptr1);
	free(ptr2);

	test("neq failure") { assertneq("hello", "hello"); }
	test("neq success") { assertneq("hello", "world"); }

	char str1[] = "hello";
	char str2[] = "world";
	test("eq success") { assertneq(str1, str1); }
	test("eq success") { assertneq(str1, str2); }

	void *vptr1 = NULL;
	void *vptr2 = (void *)0x10;
	test("eq success") { assertneq(vptr1, vptr1); }
	test("eq success") { assertneq(vptr1, vptr2); }
}

snow_main();
