#include "vector.h"
#include <stdio.h>

#ifdef SNOW_ENABLED

#include <snow/snow.h>
snow_main();

#else

// Example program which will just put all strings in argv into a vector,
// then loop through and print the arguments
int main(int argc, char **argv) {
	vector vec;
	vector_init(&vec, sizeof(char *));

	for (int i = 0; i < argc; ++i)
		vector_set(&vec, i, &argv[i]);

	printf("Got %zi arguments:\n", vec.count);
	for (size_t i = 0; i < vec.count; ++i)
		printf("%s\n", *(char **)vector_get(&vec, i));

	vector_free(&vec);

	return 0;
}

#endif
