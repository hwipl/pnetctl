#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "pnetctl.h"
#include "netlink.h"
#include "devices.h"
#include "udev.h"

/* verbose output, disabled by default */
int verbose_mode = 0;

/* pnetid filter when printing the device table */
const char *pnetid_filter = NULL;

/* print verbose output to the screen if in verbose mode */
void verbose(const char *format, ...) {
	va_list args;

	va_start(args, format);
	if (verbose_mode)
		vprintf(format, args);
	va_end(args);
}

/*
 * *****************
 * *** MAIN PART ***
 * *****************
 */

/* print a horizontal line on screen */
void print_line() {
	printf("----------------------------------------------------------");
	printf("----------\n");
}

/* print a horizontal bold line on screen */
void print_bold_line() {
	printf("==========================================================");
	printf("==========\n");
}

/* print the header on screen */
void print_header() {
	print_bold_line();
	printf("%-16s %5.5s %15.15s %6.6s %5.5s %16.16s\n", "Pnetid:", "Type:",
	       "Name:", "Port:", "Bus:", "Bus-ID:");
	print_bold_line();
}

/* print the pnetid on screen */
void print_pnetid(char *pnetid) {
	printf("%s\n", pnetid);
	print_line();
}

/* print device on screen */
void print_device(struct device *device) {
	printf("%-16s", "");
	if (!strncmp(device->subsystem, "infiniband", 10)) {
		printf(" %5.5s", "ib");
		printf(" %15.15s", device->name);
		printf(" %6d", device->ib_port);
	} else {
		printf(" %5.5s", device->subsystem);
		printf(" %15.15s", device->name);
		printf(" %6.6s", "");
	}
	if (device->parent_subsystem)
		printf("   %3.3s", device->parent_subsystem);
	else
		printf(" %5.5s", "n/a");
	if (device->parent)
		printf(" %16.16s", device->parent);
	else
		printf(" %16.16s", "n/a");
	printf("\n");
}

/* print all devices on screen */
void print_device_table() {
	int pnetid_found = 1;
	struct device *next;
	char *pnetid;

	/* print header first */
	print_header();

	/* print each pnetid and devices with same pnetid */
	while (pnetid_found) {
		pnetid_found = 0;
		pnetid = NULL;
		next = get_next_device(&devices_list);
		while (next) {
			/* skip devices already output and without pnetid */
			if (next->output || !next->pnetid[0]) {
				next = get_next_device(next);
				continue;
			}

			/* if pnetid filter is active, only show this pnetid */
			if (pnetid_filter) {
				if (!strncmp(next->pnetid, pnetid_filter,
					     SMC_MAX_PNETID_LEN)) {
					print_device(next);
					next->output = 1;
				}
				next = get_next_device(next);
				continue;
			}

			if (pnetid_found) {
				/* another device with same pnetid? */
				if (!strncmp(next->pnetid, pnetid,
					     SMC_MAX_PNETID_LEN)) {
					print_device(next);
					next->output = 1;
				}
				next = get_next_device(next);
				continue;
			}

			/* found new pnetid in list */
			pnetid_found = 1;
			pnetid = next->pnetid;
			print_pnetid(pnetid);
			print_device(next);
			next->output = 1;
			next = get_next_device(next);
		}
		if (pnetid_found && !pnetid_filter)
			print_line();
	}

	/* print remaining devices */
	if (pnetid_filter)
		return;
	print_pnetid("n/a");
	next = get_next_device(&devices_list);
	while (next) {
		if (!next->output)
			print_device(next);
		next = get_next_device(next);
	}
}

/* print usage */
void print_usage() {
	printf("------------------------------------------------------------\n"
	       "Usage:\n"
	       "------------------------------------------------------------\n"
	       "pnetctl			Print all devices and pnetids\n"
	       "pnetctl <options>	Run commands specified by options\n"
	       "			(see below)\n"
	       "------------------------------------------------------------\n"
	       "Options:\n"
	       "------------------------------------------------------------\n"
	       "-a <pnetid>		Add pnetid. Requires -n or -i\n"
	       "-r <pnetid>		Remove pnetid\n"
	       "-g <pnetid>		Get devices with pnetid\n"
	       "-f			Flush pnetids\n"
	       "-n <name>		Specify net device\n"
	       "-i <name>		Specify infiniband or ism device\n"
	       "-p <port>		Specify infiniband port\n"
	       "			(default: %d)\n"
	       "-v			Print verbose output\n"
	       "-h			Print this help\n",
	       IB_DEFAULT_PORT
	       );
}

/* run the "flush" command to remove all pnetid entries */
int run_flush_command() {
	/* remove entries via netlink */
	nl_init();
	nl_flush_pnetids();
	nl_cleanup();
	return EXIT_SUCCESS;
}

/* run the "remove"/"del" command to remove a pnetid entry */
int run_del_command(const char *pnetid) {
	/* remove entry via netlink */
	nl_init();
	nl_del_pnetid(pnetid);
	nl_cleanup();
	return EXIT_SUCCESS;
}

/* run the "add" command to add a pnetid entry */
int run_add_command(const char *pnetid, const char *net_device,
		    const char *ib_device, int ib_port) {
	/* at least one device must be present */
	if (!ib_device && !net_device) {
		verbose("Missing ib or net device.\n");
		print_usage();
		return EXIT_FAILURE;
	}

	/* add entry via netlink */
	nl_init();
	nl_set_pnetid(pnetid, net_device, ib_device, ib_port);
	nl_cleanup();
	return EXIT_SUCCESS;
}

/* run the "get" command to get devices and pnetids */
int run_get_command() {
	int rc;

	/* get all devices via udev and put them in devices list */
	verbose("Trying to find devices and read their pnetids from "
		"util_strings.\n");
	rc = udev_scan_devices();
	if (rc)
		return rc;

	/* try to receive pnetids via netlink */
	verbose("Trying to read pnetids via netlink.\n");
	nl_init();
	nl_get_pnetids();
	nl_cleanup();

	/* print devices to the screen, cleanup, and exit */
	verbose("Printing device table.\n");
	print_device_table();
	free_devices();
	return EXIT_SUCCESS;
}

/* parse command line arguments and call other functions */
int parse_cmd_line(int argc, char **argv) {
	char *net_device = NULL;
	char *ib_device = NULL;
	char *pnetid = NULL;
	char ib_port = -1;
	int remove = 0;
	int flush = 0;
	int add = 0;
	int get = 0;
	int c;

	/* try to get all arguments */
	while ((c = getopt (argc, argv, "a:fhi:n:p:r:g:v")) != -1) {
		switch (c) {
		case 'a':
			add = 1;
			pnetid = optarg;
			break;
		case 'f':
			flush = 1;
			break;
		case 'r':
			remove = 1;
			pnetid = optarg;
			break;
		case 'g':
			get = 1;
			pnetid = optarg;
			break;
		case 'n':
			net_device = optarg;
			break;
		case 'i':
			ib_device = optarg;
			break;
		case 'p':
			ib_port = atoi(optarg);
			break;
		case 'v':
			verbose_mode = 1;
			break;
		case 'h':
			print_usage();
			return EXIT_SUCCESS;
		default:
			goto fail;
		}
	}

	/* check for conflicting command line parameters */
	if ((add && flush) || (add && remove) || (remove && flush) ||
	    (get && add) || (get && remove) || (get && flush)) {
		verbose("Conflicting command line arguments.\n");
		goto fail;
	}

	if (flush) {
		/* flush all pnetids and quit */
		verbose("Flushing all pnetids.\n");
		return run_flush_command();
	}

	if (remove) {
		/* remove a specific pnetid */
		verbose("Removing pnetid \"%s\".\n", pnetid);
		return run_del_command(pnetid);
	}

	if (add) {
		/* add a pnetid entry */
		verbose("Adding pnetid \"%s\".\n", pnetid);
		return run_add_command(pnetid, net_device, ib_device, ib_port);
	}

	if (get) {
		/* get a specific pnetid */
		verbose("Getting devices with pnetid \"%s\".\n", pnetid);
		pnetid_filter = pnetid;
		return run_get_command();
	}

	/* No special commands, print device table to screen if there was
	 * no command line argument or if we are in verbose mode
	 */
	if (argc == 1 || verbose_mode) {
		/* get all devices and pnetids */
		verbose("Getting all devices and pnetids.\n");
		return run_get_command();
	}
fail:
	print_usage();
	return EXIT_FAILURE;
}

/* main function */
int main(int argc, char **argv) {
	int rc;

	/* parse command line arguments and run everything else from there */
	rc = parse_cmd_line(argc, argv);

	exit(rc);
}
