/*
 * test for devices
 */

#include <string.h>

#include "devices.h"

// test the function new_device()
int test_new_device() {
	struct device *device = new_device();
	struct device *next;

	next = get_next_device(&devices_list);
	while (next) {
		if (next == device) {
			return 0;
		}
		next = get_next_device(next);
	}

	free_devices();
	return -1;
}

// test the function get_next_device()
int test_get_next_device() {
	struct device *device;
	device = get_next_device(&devices_list);
	while (device) {
		device = get_next_device(device);
	}
	free_devices();
	return 0;
}

// test the function set_pnetid_for_eth()
int test_set_pnetid_for_eth() {
	struct device *device = new_device();
	device->name = "lo";
	device->subsystem = "net";
	set_pnetid_for_eth("lo", "PNETID");
	if (strncmp(device->pnetid, "PNETID", sizeof(device->pnetid))) {
	    return -1;
	}
	free_devices();
	return 0;
}

int main() {
	int rc;

	rc = test_new_device();
	if (rc) {
		return rc;
	}
	rc = test_get_next_device();
	if (rc) {
		return rc;
	}
	rc = test_set_pnetid_for_eth();
	if (rc) {
		return rc;
	}
}
