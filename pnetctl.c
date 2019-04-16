#include <libudev.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * ********************
 * *** DEVICES PART ***
 * ********************
 */

/* struct for devices */
struct device {
	/* list */
	struct device *next;

	/* udev */
	struct udev_device *udev_device;
	struct udev_device *udev_parent;
	struct udev_device *udev_lowest;

	/* names */
	const char *subsystem;
	const char *name;
	const char *parent;
	const char *lowest;
};

/* list of devices */
struct device devices_list = {0};

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

	next = get_next_device(&devices_list);
	while (next) {
		// TODO: also free udev devices?
		cur = next;
		next = get_next_device(next);
		free(cur);
	}
}

/*
 * *****************
 * *** UDEV PART ***
 * *****************
 */

/* udev return codes */
enum udev_rc {
	UDEV_OK,
	UDEV_FAILED,
	UDEV_ENUM_FAILED,
	UDEV_MATCH_FAILED,
	UDEV_SCAN_FAILED,
	UDEV_DEV_FAILED,
	UDEV_HANDLE_FAILED,
};

/* handle a device found by scan_devices() */
int handle_device(struct udev_device *udev_device,
		  struct udev_device *udev_parent,
		  struct udev_device *udev_lowest) {
	struct device *device;

	device = new_device();
	if (!device)
		return UDEV_HANDLE_FAILED;

	device->udev_device = udev_device;
	device->udev_parent = udev_parent;
	device->udev_lowest = udev_lowest;

	device->name = udev_device_get_sysname(udev_device);
	device->subsystem = udev_device_get_subsystem(udev_device);
	device->parent = udev_device_get_sysname(udev_parent);
	device->lowest = udev_device_get_sysname(udev_lowest);

	return 0;
}

/* find "lower" device of a net device */
struct udev_device *udev_find_lower(struct udev_device *udev_device) {
	struct udev_list_entry *next;
	const char *lower_name = NULL;
	struct udev_device *lower_dev;
	struct udev *udev_ctx;

	next = udev_device_get_sysattr_list_entry(udev_device);
	while (next) {
		const char *name = udev_list_entry_get_name(next);
		if (!strncmp(name, "lower_", 6)) {
			lower_name = name + 6;
			break;
		}
		next = udev_list_entry_get_next(next);
	}

	if (!lower_name)
		return NULL;
	udev_ctx = udev_device_get_udev(udev_device);
	lower_dev = udev_device_new_from_subsystem_sysname(udev_ctx, "net",
							   lower_name);
	return lower_dev;
}

/* find lowest "lower" device of a net device */
struct udev_device *udev_find_lowest(struct udev_device *udev_device) {
	struct udev_device *lowest = udev_device;
	struct udev_device *lower;

	lower = udev_find_lower(udev_device);
	while (lower) {
		// TODO: unref lower at some point?
		lowest = lower;
		lower = udev_find_lower(lower);
	}

	return lowest;
}

/* handle the found udev device */
int udev_handle_device(struct udev_device *udev_device) {
	struct udev_device *udev_parent = NULL;
	struct udev_device *udev_lowest = NULL;
	const char *subsystem;
	int rc;

	subsystem = udev_device_get_subsystem(udev_device);
	udev_parent = udev_device_get_parent(udev_device);

	if (!strncmp(subsystem, "net", 3))
		udev_lowest = udev_find_lowest(udev_device);

	rc = handle_device(udev_device, udev_parent, udev_lowest);
	if (rc)
		return rc;

	return UDEV_OK;
}

/* scan devices and call handle_device on each */
int udev_scan_devices() {
	struct udev_enumerate *udev_enum;
	struct udev_list_entry *next;
	struct udev *udev_ctx;

	udev_ctx = udev_new();
	if (!udev_ctx)
		return UDEV_FAILED;

	udev_enum = udev_enumerate_new(udev_ctx);
	if (!udev_enum)
		return UDEV_ENUM_FAILED;

	if (udev_enumerate_add_match_subsystem(udev_enum, "infiniband"))
		return UDEV_MATCH_FAILED;

	if (udev_enumerate_add_match_subsystem(udev_enum, "net"))
		return UDEV_MATCH_FAILED;

	if (udev_enumerate_scan_devices(udev_enum) < 0)
		return UDEV_SCAN_FAILED;

	/* enumerate all devices and handle them */
	next = udev_enumerate_get_list_entry(udev_enum);
	while (next) {
		const char *name = udev_list_entry_get_name(next);
		struct udev_device *udev_device;
		int rc;

		udev_device = udev_device_new_from_syspath(udev_ctx, name);
		if (!udev_device)
			return UDEV_DEV_FAILED;

		rc = udev_handle_device(udev_device);
		if (rc)
			return rc;
		next = udev_list_entry_get_next(next);
	}
	return UDEV_OK;
}

/*
 * *****************
 * *** MAIN PART ***
 * *****************
 */

/* main function */
int main() {
	struct device *next;
	int rc;

	/* get all devices via udev and put them in devices list */
	rc = udev_scan_devices();
	if (rc)
		return rc;

	/* handle each device in devices list */
	printf("%10.10s %15.15s %16.16s %15.15s\n", "Type:", "Name:", "PCI-ID",
	       "Lowest");
	next = get_next_device(&devices_list);
	while (next) {
		printf("%10.10s ", next->subsystem);
		printf(" %15.15s", next->name);
		if (next->parent)
			printf(" %16.16s", next->parent);
		else
			printf(" %16.16s", "n/a");
		if (next->lowest)
			printf(" %15.15s", next->lowest);
		else
			printf(" %15.15s", "n/a");
		printf("\n");

		next = get_next_device(next);
	}

	/* free devices in devices list */
	free_devices();

	return 0;
}
