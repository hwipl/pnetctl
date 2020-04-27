/*
 * *****************
 * *** UDEV PART ***
 * *****************
 */

#include <libudev.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iconv.h>
#include <string.h>
#include <stdlib.h>

#include "pnetctl.h"
#include "devices.h"

#define DEV_TYPE_ISM "ism" /* device type for ISM devices */

/* CCW device constants */
#define CCW_CHPID_LEN 128 /* maximum chpid length */
#define CCW_UTIL_PREFIX "/sys/devices/css0/chp0." /* util string path prefix */

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

/* read pnetid from util_string into buffer */
int read_util_string(const char *file, char *buffer) {
	char read_buffer[SMC_MAX_PNETID_LEN];
	char *read_ptr = read_buffer;
	size_t read_count;
	size_t conv_count;
	iconv_t cd;
	int fd;
	int rc;

	/* open and read file to temporary buffer*/
	verbose("Reading util string from file \"%s\".\n", file);
	fd  = open(file, O_RDONLY);
	if (fd == -1)
		return -1;
	read_count = read(fd, read_buffer, SMC_MAX_PNETID_LEN);
	close(fd);
	if (read_count == -1)
		return -1;

	/* initialize ebcdic to ascii converter */
	cd = iconv_open("ASCII", "CP500");
	if (cd == (iconv_t) -1)
		return -1;

	/* convert pnetid from ebcdic to ascii; write to output buffer */
	conv_count = SMC_MAX_PNETID_LEN;
	rc = iconv(cd, &read_ptr, &read_count, &buffer, &conv_count);
	iconv_close(cd);
	if (rc == -1)
		return -1;

	verbose("Read util string \"%s\" from file \"%s\".\n", buffer, file);
	return 0;
}

/* helper for finding util strings of pci devices */
int find_pci_util_string(struct device *device) {
	const char *udev_path = udev_device_get_syspath(device->udev_parent);
	int path_len = strlen(udev_path) + strlen("/util_string") + 1;
	char util_string_path[path_len];
	struct udev_list_entry *next;

	verbose("Trying to find util_string for pci device \"%s\".\n",
		device->name);

	next = udev_device_get_sysattr_list_entry(device->udev_parent);
	while (next) {
		const char *name = udev_list_entry_get_name(next);
		if (!strncmp(name, "util_string", 11)) {
			snprintf(util_string_path, sizeof(util_string_path),
				 "%s/util_string", udev_path);
			read_util_string(util_string_path, device->pnetid);
			break;
		}
		next = udev_list_entry_get_next(next);
	}

	return 0;
}

/* helper for finding util strings of ccwgroup devices */
int find_ccw_util_string(struct device *device) {
	const char *udev_path = udev_device_get_syspath(device->udev_parent);
	int util_path_len = strlen(CCW_UTIL_PREFIX) + CCW_CHPID_LEN +
		strlen("/util_string") + 1;
	int chpid_path_len = strlen(udev_path) + strlen("/chpid") + 1;
	char util_string_path[util_path_len];
	char chpid[CCW_CHPID_LEN + 1] = {0};
	char chpid_path[chpid_path_len];
	int count;
	int fd;

	verbose("Trying to find util_string for ccw device \"%s\".\n",
		device->name);

	/* try to read chpid */
	snprintf(chpid_path, sizeof(chpid_path), "%s/chpid", udev_path);
	verbose("Reading chpid from file \"%s\".\n", chpid_path);
	fd = open(chpid_path, O_RDONLY);
	if (fd == -1)
		return -1;
	count = read(fd, chpid, CCW_CHPID_LEN);
	close(fd);
	if (count <= 0)
		return count;
	chpid[strcspn(chpid, "\r\n")] = 0;
	verbose("Read chpid \"%s\" from file \"%s\".\n", chpid, chpid_path);

	/* try to read util string */
	snprintf(util_string_path, sizeof(util_string_path), "%s%s/util_string",
		 CCW_UTIL_PREFIX, chpid);
	return read_util_string(util_string_path, device->pnetid);
}

/* try to find a util_string for the device and read the pnetid */
int find_util_string(struct device *device) {
	/* pci device */
	if (device->parent_subsystem &&
	    !strncmp(device->parent_subsystem, "pci", 3))
		find_pci_util_string(device);

	/* ccw group device */
	if (device->parent_subsystem &&
	    !strncmp(device->parent_subsystem, "ccwgroup", 8))
		find_ccw_util_string(device);

	return 0;
}

/* handle device helper */
struct device *_handle_device(struct udev_device *udev_device,
			      struct udev_device *udev_parent,
			      struct udev_device *udev_lowest,
			      int ib_port) {
	struct device *device;

	device = new_device();
	if (!device)
		return NULL;

	/* initialize struct members */
	device->udev_device = udev_device;
	device->udev_parent = udev_parent;
	device->udev_lowest = udev_lowest;

	device->name = udev_device_get_sysname(udev_device);
	device->subsystem = udev_device_get_subsystem(udev_device);
	device->parent = udev_device_get_sysname(udev_parent);
	device->parent_subsystem = udev_device_get_subsystem(udev_parent);
	device->lowest = udev_device_get_sysname(udev_lowest);
	device->ib_port = ib_port;
	verbose("Added device \"%s\" to device table.\n", device->name);

	/* try to initialize pnetid from util_string */
	find_util_string(device);

	return device;
}

/* handle a device found by scan_devices() */
int handle_device(struct udev_device *udev_device,
		  struct udev_device *udev_parent,
		  struct udev_device *udev_lowest,
		  int ib_port) {
	if (_handle_device(udev_device, udev_parent, udev_lowest, ib_port))
		return 0;

	return UDEV_HANDLE_FAILED;
}

/* handle an ism device found by scan_devices() */
int handle_ism_device(struct udev_device *udev_device) {
	struct device *device;

	device = _handle_device(udev_device, udev_device, NULL, -1);
	if (!device)
		return UDEV_HANDLE_FAILED;
	device->subsystem = DEV_TYPE_ISM;
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

/* find infiniband ports of a udev device */
int udev_find_ibports(struct udev_device *udev_device, int *first, int *last) {
	const char *udev_path = udev_device_get_syspath(udev_device);
	int path_len = strlen(udev_path) + strlen("/ports") + 1;
	char ports_dir[path_len];
	struct dirent *dir_ent;
	int num_ports = 0;
	DIR *dir;

	/* construct directory path, read directory, and extract ib ports */
	snprintf(ports_dir, sizeof(ports_dir), "%s/ports", udev_path);
	dir = opendir(ports_dir);
	if (dir) {
		*first = -1;
		*last = -1;
		while ((dir_ent = readdir(dir)) != NULL) {
			if (!strncmp(dir_ent->d_name, ".", 1))
				continue;
			num_ports++;
			if (*first == -1)
				*first = atoi(dir_ent->d_name);
			*last = atoi(dir_ent->d_name);
		}
		closedir(dir);
	}

	return num_ports;
}

/* handle the found udev device */
int udev_handle_device(struct udev_device *udev_device) {
	struct udev_device *udev_parent = NULL;
	struct udev_device *udev_lowest = NULL;
	int ib_port_first = -1;
	int ib_port_last = -1;
	const char *subsystem;
	const char * driver;
	int ib_ports = 0;
	int rc = 0;

	subsystem = udev_device_get_subsystem(udev_device);
	udev_parent = udev_device_get_parent(udev_device);

	if (!strncmp(subsystem, "net", 3)) {
		udev_lowest = udev_find_lowest(udev_device);
		rc = handle_device(udev_device, udev_parent, udev_lowest, -1);
	}

	if (!strncmp(subsystem, "infiniband", 10)) {
	       ib_ports = udev_find_ibports(udev_device, &ib_port_first,
					    &ib_port_last);
	       for (int i = ib_port_first; i < ib_port_first + ib_ports; i++)
		       rc = handle_device(udev_device, udev_parent, udev_lowest,
					  i);
	}

	if (!strncmp(subsystem, "pci", 3)) {
		driver = udev_device_get_driver(udev_device);
		if (driver && !strncmp(driver, "ism", 3))
			rc = handle_ism_device(udev_device);
	}

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

	if (udev_enumerate_add_match_subsystem(udev_enum, "pci"))
		return UDEV_MATCH_FAILED;

	verbose("Scanning devices with udev.\n");
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
