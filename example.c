#include <stdio.h>
#include "test/test.h"

describe(files, {
	it("opens files", {
		FILE *f = fopen("test", "r");
		assertneq(f, NULL);
		defer(fclose(f));
	});

	it("writes to files", {
		FILE *f = fopen("testfile", "w");
		assertneq(f, NULL);
		defer(remove("testfile"));
		defer(fclose(f));

		char *str = "hello there";
		asserteq(fwrite(str, 1, sizeof(str), f), sizeof(str));
	});

	subdesc(fread, {
		it("reads 10 bytes", {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[10];
			asserteq(fread(buf, 1, 10, f), 10);
		});

		it("reads 20 bytes", {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[20];
			asserteq(fread(buf, 1, 20, f), 20);
		});

		it("reads 200000 bytes", {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[200000];
			asserteq(fread(buf, 1, 200000, f), 200000);
		});

		it("reads 23 bytes", {
			FILE *f = fopen("/dev/zero", "r");
			assertneq(f, NULL);
			defer(fclose(f));

			char buf[23];
			asserteq(fread(buf, 1, 23, f), 23);
		});
	});
});

int main() {
	test_files();
}
