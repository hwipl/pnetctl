#ifndef _PNETCTL_DEVICES_H
#define _PNETCTL_DEVICES_H

#include "common.h"

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
	const char *parent_subsystem;
	const char *lowest;

	/* infiniband */
	int ib_port;

	/* pnetid */
	char pnetid[SMC_MAX_PNETID_LEN + 1];

	/* terminal output */
	int output;
};

/* list of devices */
struct device devices_list;

struct device *new_device();
struct device *get_next_device(struct device *device);
void set_pnetid_for_eth(const char *dev_name, const char* pnetid);
void set_pnetid_for_ib(const char *dev_name, int dev_port, const char* pnetid);
void free_devices();

#endif
