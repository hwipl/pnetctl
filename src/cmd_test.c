/*
 * test for cmd
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "test.h"
#include "cmd.h"

// test the function parse_cmd_line()
int test_parse_cmd_line() {
	char *exe = "pnetctl";
	int rc;

	// no command line arguments
	char *args_none[] = {exe};
	rc = parse_cmd_line(1, args_none);
	if (rc) {
		return rc;
	}

	// unknown command line argument
	char *args_unknown[] = {exe, "UNKNOWN"};
	rc = parse_cmd_line(2, args_unknown);
	if (!rc) {
		return -1;
	}

	// help
	char *args_help[] = {exe, "-h"};
	rc = parse_cmd_line(2, args_help);
	if (rc) {
		return rc;
	}

	// add
	char *args_add[] = {exe, "-a", "PNETCTL", "-n", "lo"};
	rc = parse_cmd_line(5, args_add);
	if (rc) {
		return rc;
	}

	// get
	char *args_get[] = {exe, "-g", "PNETCTL"};
	rc = parse_cmd_line(3, args_get);
	if (rc) {
		return rc;
	}

	// remove
	char *args_remove[] = {exe, "-r", "PNETCTL"};
	rc = parse_cmd_line(3, args_remove);
	if (rc) {
		return rc;
	}

	// flush
	char *args_flush[] = {exe, "-f"};
	rc = parse_cmd_line(2, args_flush);
	if (rc) {
		return rc;
	}

	// no command line arguments, verbose
	char *args_none_verbose[] = {exe, "-v"};
	rc = parse_cmd_line(2, args_none_verbose);
	if (rc) {
		return rc;
	}

	// unknown command line argument, verbose
	char *args_unknown_verbose[] = {exe, "-v", "UNKNOWN"};
	rc = parse_cmd_line(3, args_unknown_verbose);
	if (rc) {
		return rc;
	}

	// help, verbose
	char *args_help_verbose[] = {exe, "-v", "-h"};
	rc = parse_cmd_line(3, args_help_verbose);
	if (rc) {
		return rc;
	}

	// add, verbose
	char *args_add_verbose[] = {exe, "-v", "-a", "PNETCTL", "-n", "lo"};
	rc = parse_cmd_line(6, args_add_verbose);
	if (rc) {
		return rc;
	}

	// get, verbose
	char *args_get_verbose[] = {exe, "-v", "-g", "PNETCTL"};
	rc = parse_cmd_line(4, args_get_verbose);
	if (rc) {
		return rc;
	}

	// remove, verbose
	char *args_remove_verbose[] = {exe, "-v", "-r", "PNETCTL"};
	rc = parse_cmd_line(4, args_remove_verbose);
	if (rc) {
		return rc;
	}

	// flush, verbose
	char *args_flush_verbose[] = {exe, "-v", "-f"};
	rc = parse_cmd_line(3, args_flush_verbose);
	if (rc) {
		return rc;
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
