/*
 * test for netlink
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "netlink.h"

// test the function nl_init()
int test_nl_init() {
	nl_init();
	nl_init();
	nl_cleanup();
	return 0;
}

// test the function nl_cleanup()
int test_nl_cleanup() {
	nl_init();
	nl_cleanup();
	return 0;
}

// test the function nl_flush_pnetids()
int test_nl_flush_pnetids() {
	nl_init();
	nl_set_pnetid("PNETCTL", "lo", NULL, 0);
	nl_flush_pnetids();
	nl_flush_pnetids();
	nl_cleanup();
	return 0;
}

// test the function nl_del_pnetid()
int test_nl_del_pnetid() {
	nl_init();
	nl_set_pnetid("PNETCTL", "lo", NULL, 0);
	nl_del_pnetid("PNETCTL");
	nl_del_pnetid("PNETCTL");
	nl_cleanup();
	return 0;
}

// test the function nl_set_pnetid()
int test_nl_set_pnetid() {
	nl_init();
	nl_set_pnetid("PNETCTL", "lo", NULL, 0);
	nl_set_pnetid("PNETCTL", NULL, "mlx5_1", 1);
	nl_set_pnetid("PNETCTL", "lo", "mlx5_1", 1);
	nl_del_pnetid("PNETCTL");
	nl_cleanup();
	return 0;
}

// test the function nl_get_pnetids()
int test_nl_get_pnetids() {
	nl_init();
	nl_get_pnetids();
	nl_set_pnetid("PNETCTL", "lo", NULL, 0);
	nl_get_pnetids();
	nl_del_pnetid("PNETCTL");
	nl_cleanup();
	return 0;
}

struct test tests[] = {
	{"nl_init", test_nl_init},
	{"nl_cleanup", test_nl_cleanup},
	{"nl_flush_pnetids", test_nl_flush_pnetids},
	{"nl_del_pnetid", test_nl_del_pnetid},
	{"nl_set_pnetid", test_nl_set_pnetid},
	{"nl_get_pnetids", test_nl_get_pnetids},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
