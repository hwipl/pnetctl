#ifndef _PNETCTL_TEST_H
#define _PNETCTL_TEST_H

#include <string.h>
#include <stdio.h>

/* test structure: name of test and the test's function */
struct test {
	const char *name;
	int (* func)();
};


/* run a specific test with name in tests or all tests if name is NULL */
int run_test(struct test tests[], const char *name);

#endif
