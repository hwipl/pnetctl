/*
 * ********************
 * *** DEVICES PART ***
 * ********************
 */

#include <string.h>
#include <stdlib.h>

#include "devices.h"
#include "verbose.h"

/* list of devices */
struct device devices_list = {};

/* get next device in devices list */
struct device *get_next_device(struct device *device) {
	return device->next;
}

/* create a new device in devices list */
struct device *new_device() {
	struct device *device;
	struct device *slot;
	struct device *next;

	device = malloc(sizeof(*device));
	memset(device, 0, sizeof(*device));

	/* add it to devices list */
	slot = &devices_list;
	next = get_next_device(&devices_list);
	while (next) {
		slot = next;
		next = get_next_device(next);
	}
	slot->next = device;

	return device;
}

/* free all devices in devices list */
void free_devices() {
	struct device *next;
	struct device *cur;

	verbose("Freeing devices in device table.\n");
	next = get_next_device(&devices_list);
	while (next) {
		// TODO: also free udev devices?
		cur = next;
		next = get_next_device(next);
		free(cur);
	}
	devices_list.next = NULL;
}

/* set pnetid for eth device */
void set_pnetid_for_eth(const char *dev_name, const char* pnetid) {
	struct device *next;

	next = get_next_device(&devices_list);
	while (next) {
		if (!strncmp(next->subsystem, "net", 3)) {
			// TODO: use strncmp?
			if (!strcmp(next->name, dev_name) ||
			    !strcmp(next->lowest, dev_name)) {
				strncpy(next->pnetid, pnetid,
					SMC_MAX_PNETID_LEN);
				verbose("Set pnetid of net device \"%s\" "
					"to \"%s\".\n", dev_name, pnetid);
			}

		}
		next = get_next_device(next);
	}
}

/* set pnetid for eth device */
void set_pnetid_for_ib(const char *dev_name, int dev_port, const char* pnetid) {
	struct device *next;

	next = get_next_device(&devices_list);
	while (next) {
		if (!strncmp(next->subsystem, "infiniband", 10)) {
			// TODO: use strncmp?
			if ((!strcmp(next->name, dev_name) ||
			     !strcmp (next->parent, dev_name)) &&
			    next->ib_port == dev_port) {
				strncpy(next->pnetid, pnetid,
					SMC_MAX_PNETID_LEN);
				verbose("Set pnetid of ib device \"%s\" "
					"to \"%s\".\n", dev_name, pnetid);
			}

		}
		next = get_next_device(next);
	}
}
