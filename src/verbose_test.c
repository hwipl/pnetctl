/*
 * test for verbose
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "verbose.h"

// test the function verbose()
int test_verbose() {
	verbose_mode = 0;
	verbose("");
	verbose("Hello World\n");
	verbose("Hello %s\n", "World");
	verbose("Verbose_mode: %d\n", verbose_mode);

	verbose_mode = 1;
	verbose("");
	verbose("Hello World\n");
	verbose("Hello %s\n", "World");
	verbose("Verbose_mode: %d\n", verbose_mode);

	return 0;
}

struct test tests[] = {
	{"verbose", test_verbose},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
