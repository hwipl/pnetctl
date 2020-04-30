/*
 * test for devices
 */

#include "test.h"
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

// test the function set_pnetid_for_ib()
int test_set_pnetid_for_ib() {
	struct device *device = new_device();
	device->name = "mlx5_1";
	device->subsystem = "infiniband";
	device->ib_port = 1;
	set_pnetid_for_ib("mlx5_1", 1, "PNETID");
	if (strncmp(device->pnetid, "PNETID", sizeof(device->pnetid))) {
	    return -1;
	}
	free_devices();
	return 0;
}

// test the function free_devices()
int test_free_devices() {
	struct device *device;
	new_device();
	device = get_next_device(&devices_list);
	if (!device) {
		return -1;
	}
	free_devices();
	device = get_next_device(&devices_list);
	if (device) {
		return -1;
	}
	free_devices();
	return 0;
}

struct test tests[] = {
	{"new_device", test_new_device},
	{"get_next_device", test_get_next_device},
	{"set_pnetid_for_eth", test_set_pnetid_for_eth},
	{"set_pnetid_for_ib", test_set_pnetid_for_ib},
	{"free_devices", test_free_devices},
	{NULL, NULL},
};

int main(int argc, char **argv) {
	const char *name = NULL;
	if (argc > 1) {
		name = argv[1];
	}
	return run_test(tests, name);
}
