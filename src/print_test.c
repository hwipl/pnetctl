/*
 * test for print
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "print.h"
#include "common.h"
#include "udev.h"
#include "netlink.h"

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

	// add a pnetid and get pnetids
	nl_init();
	nl_set_pnetid("PNETCTL", "lo", NULL, 0);
	nl_get_pnetids();

	// filled device table, no filter
	pnetid_filter = NULL;
	print_device_table();

	// filled device table, filter
	pnetid_filter = "PNETCTL";
	print_device_table();

	// cleanup
	nl_del_pnetid("PNETCTL");
	nl_cleanup();

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
