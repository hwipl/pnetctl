/*
 * test for devices
 */

#include <string.h>
#include <stdio.h>

#include "udev.h"

// test the function udev_scan_devices()
int test_udev_scan_devices() {
	return udev_scan_devices();
}

struct test {
	const char *name;
	int (* func)();
};

struct test tests[] = {
	{"udev_scan_devices", test_udev_scan_devices},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	int rc;

	// no command line argument -> run all tests
	if (argc == 1) {
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
		if (!strcmp(tests[i].name, argv[1])) {
			printf("Testing %s.\n", tests[i].name);
			return tests[i].func();
		}
	}
	printf("Test not found.\n");
	return -1;
}
