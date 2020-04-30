/*
 * test for print
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "print.h"
#include "common.h"
#include "udev.h"

// test the function print_device_table()
int test_print_device_table() {
	// empty device table, no filter
	pnetid_filter = NULL;
	print_device_table();

	// empty device table, (not matching) filter
	pnetid_filter = "DOES_NOT_MATCH";
	print_device_table();

	// fill device table
	udev_scan_devices();

	// filled device table, no filter
	pnetid_filter = NULL;
	print_device_table();

	// filled device table, (not matching) filter
	pnetid_filter = "DOES_NOT_MATCH";
	print_device_table();

	return 0;
}

struct test tests[] = {
	{"print_device_table", test_print_device_table},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
