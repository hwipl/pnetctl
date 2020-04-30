/*
 * test for cmd
 */

#include <string.h>
#include <stdio.h>

#include "test.h"
#include "cmd.h"

// test the function parse_cmd_line()
int test_parse_cmd_line() {
	int rc;

	// no command line arguments
	char *args_none[] = {"pnetctl"};
	rc = parse_cmd_line(1, args_none);
	if (rc) {
		return rc;
	}

	// unknown command line argument
	char *args_unknown[] = {"pnetctl", "UNKNOWN"};
	rc = parse_cmd_line(2, args_unknown);
	if (!rc) {
		return -1;
	}

	return 0;
}

struct test tests[] = {
	{"parse_cmd_line", test_parse_cmd_line},
	{NULL, NULL},
};

int main(int argc, char** argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
