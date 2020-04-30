#include <stdio.h>
#include <string.h>

#include "pnetctl.h"
#include "devices.h"

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

