#include "test.h"

/* run a specific test with name in tests or all tests if name is NULL */
int run_test(struct test tests[], const char *name) {
	int rc;

	// no command line argument -> run all tests
	if (!name) {
		for (int i=0; tests[i].name != NULL; i++) {
			printf("Testing %s.\n", tests[i].name);
			rc = tests[i].func();
			if (rc) {
				return rc;
			}
		}
		return 0;
	}

	// run specific test given as first command line argument
	for (int i=0; tests[i].name != NULL; i++) {
		if (!strcmp(tests[i].name, name)) {
			printf("Testing %s.\n", tests[i].name);
			return tests[i].func();
		}
	}
	printf("Test not found.\n");
	return -1;
}

