/*
 * test for devices
 */

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
	return -1;
}

int main() {
	int rc;

	rc = test_new_device();
	if (rc) {
		return rc;
	}
}
