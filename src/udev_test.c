/*
 * test for devices
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "udev.h"

// test the function udev_scan_devices()
int test_udev_scan_devices() {
	return udev_scan_devices();
}

struct test tests[] = {
	{"udev_scan_devices", test_udev_scan_devices},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
