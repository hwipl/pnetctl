#include <libudev.h>
#include <string.h>
#include <stdio.h>

/* udev return codes */
enum udev_ret {
	UDEV_OK,
	UDEV_FAILED,
	UDEV_ENUM_FAILED,
	UDEV_MATCH_FAILED,
	UDEV_SCAN_FAILED,
	UDEV_DEV_FAILED,
};

/* handle a device found by scan_devices() */
int handle_device(const char *subsystem, const char *name, const char *pci_id,
		  const char* lowest) {
	printf("SUBSYSTEM %s ", subsystem);
	printf("SYSNAME %s ", name);
	printf("PARENT %s", pci_id);
	if (lowest)
		printf(" LOWEST %s", lowest);
	printf("\n");

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
		const char *value = udev_list_entry_get_value(next);
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
	struct udev_device *udev_parent;
	struct udev_device *udev_lowest;
	const char *udev_parent_sysname;
	const char *lowest_name = NULL;
	const char *subsystem;
	const char *sysname;
	int rc;

	sysname = udev_device_get_sysname(udev_device);
	subsystem = udev_device_get_subsystem(udev_device);
	udev_parent = udev_device_get_parent(udev_device);
	udev_parent_sysname = udev_device_get_sysname(udev_parent);

	if (!strncmp(subsystem, "net", 3)) {
		udev_lowest = udev_find_lowest(udev_device);
		if (udev_lowest != udev_device)
			lowest_name = udev_device_get_sysname(udev_lowest);
	}
	rc = handle_device(subsystem, sysname, udev_parent_sysname,
			   lowest_name);
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

/* main function */
int main() {
	int rc;

	rc = udev_scan_devices();
	return rc;
}
