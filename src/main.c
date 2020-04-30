#include <stdlib.h>

#include "cmd.h"

/* main function */
int main(int argc, char **argv) {
	int rc;

	/* parse command line arguments and run everything else from there */
	rc = parse_cmd_line(argc, argv);

	exit(rc);
}
