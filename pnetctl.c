#include <libudev.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <rdma/ib_user_verbs.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <linux/smc.h>
#include <dirent.h>

#define SMC_MAX_PNETID_LEN 16 /* maximum length of pnetids */

void set_pnetid_for_ib(const char *dev_name, int dev_port, const char* pnetid);
void set_pnetid_for_eth(const char *dev_name, const char* pnetid);

/*
 * ********************
 * *** NETLINK PART ***
 * ********************
 */

struct nl_sock *nl_sock;
int nl_version;
int nl_family;

/* netlink policy for pnetid attributes */
static struct nla_policy smc_pnet_policy[SMC_PNETID_MAX + 1] = {
	[SMC_PNETID_NAME] = {
		.type = NLA_NUL_STRING,
		/* TODO: make sure +1 is correct */
		.maxlen = SMC_MAX_PNETID_LEN + 1
	},
	[SMC_PNETID_ETHNAME] = {
		.type = NLA_NUL_STRING,
		/* TODO: make sure it is not -1 */
		.maxlen = IFNAMSIZ
	},
	[SMC_PNETID_IBNAME] = {
		.type = NLA_NUL_STRING,
		/* TODO: make sure it is not -1 */
		.maxlen = IB_DEVICE_NAME_MAX
	},
	[SMC_PNETID_IBPORT] = { .type = NLA_U8 }
};

/* receive and parse netlink message */
int nl_parse_msg(struct nl_msg *msg, void *arg) {
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct nlattr *attrs[SMC_PNETID_MAX + 1];

	if (genlmsg_parse(hdr, 0, attrs, SMC_PNETID_MAX, smc_pnet_policy) < 0) {
		printf("Error parsing netlink attributes\n");
		return NL_STOP;
	}

	if (!attrs[SMC_PNETID_NAME]) {
		/* pnetid name is not present in message, abort */
		return NL_OK;
	}
	if (attrs[SMC_PNETID_ETHNAME]) {
		/* eth name is present in message */
		set_pnetid_for_eth(nla_get_string(attrs[SMC_PNETID_ETHNAME]),
				   nla_get_string(attrs[SMC_PNETID_NAME]));
	}
	if (attrs[SMC_PNETID_IBNAME]) {
		/* ib name is present in message */
		if (!attrs[SMC_PNETID_IBPORT]) {
			/* ib port is not present in message, abort */
			printf("Error retrieving netlink IB attributes\n");
			return NL_OK;
		}
		set_pnetid_for_ib(nla_get_string(attrs[SMC_PNETID_IBNAME]),
				  nla_get_u8(attrs[SMC_PNETID_IBPORT]),
				  nla_get_string(attrs[SMC_PNETID_NAME]));
	}
	return NL_OK;
}

/* flush all pnetids */
void nl_flush_pnetids() {
	genl_send_simple(nl_sock, nl_family, SMC_PNETID_FLUSH, nl_version, 0);
	nl_recvmsgs_default(nl_sock);
}

/* get all pnetids */
void nl_get_pnetids() {
	genl_send_simple(nl_sock, nl_family, SMC_PNETID_GET, nl_version,
			 NLM_F_DUMP);
	/* check reply */
	nl_recvmsgs_default(nl_sock);
}

/* set pnetid */
void nl_send_pnetid(const char *pnet_name, const char *eth_name,
		    const char *ib_name, char ib_port) {
	struct nl_msg* msg;
	int rc;

	/* construct netlink message */
	msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, nl_family, 0, NLM_F_REQUEST,
		    SMC_PNETID_ADD, nl_version);
	nla_put_string(msg, SMC_PNETID_NAME, pnet_name);
	nla_put_string(msg, SMC_PNETID_ETHNAME, eth_name);
	nla_put_string(msg, SMC_PNETID_IBNAME, ib_name);
	nla_put_u8(msg, SMC_PNETID_IBPORT, ib_port);

	/* send and free netlink message */
	rc = nl_send_auto(nl_sock, msg);
	if (rc < 0)
		printf("Error sending request: %d\n", rc);
	nlmsg_free(msg);

	/* check reply */
	nl_recvmsgs_default(nl_sock);
}

/* set pnetids */
void nl_set_pnetids() {
	nl_flush_pnetids();
	// Just a test... TODO: do it correctly?
	nl_send_pnetid("testid", "lo", "mlx5_0", 1);
}

/* init netlink part */
void nl_init() {
	nl_sock = nl_socket_alloc();
	nl_socket_modify_cb(nl_sock, NL_CB_VALID, NL_CB_CUSTOM, nl_parse_msg,
			    NULL);
	genl_connect(nl_sock);
	nl_family = genl_ctrl_resolve(nl_sock, SMCR_GENL_FAMILY_NAME);
	nl_version = SMCR_GENL_FAMILY_VERSION;
}

/* cleanup netlink part */
void nl_cleanup() {
	nl_close(nl_sock);
	nl_socket_free(nl_sock);
}

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

	/* infiniband */
	int ib_port;

	/* pnetid */
	char pnetid[SMC_MAX_PNETID_LEN + 1];
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
			if (!strcmp(next->name, dev_name) &&
			    next->ib_port == dev_port) {
				strncpy(next->pnetid, pnetid,
					SMC_MAX_PNETID_LEN);
			}

		}
		next = get_next_device(next);
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
		  struct udev_device *udev_lowest,
		  int ib_port) {
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
	device->ib_port = ib_port;

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
	int ib_ports = 0;
	int rc;

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

	/* try to receive pnetids via netlink */
	nl_init();
	// TODO: do it correctly
	nl_set_pnetids();
	nl_get_pnetids();
	nl_cleanup();

	/* handle each device in devices list */
	printf("%18.18s %10.10s %15.15s %16.16s %15.15s %9.9s\n", "Pnetid:",
	       "Type:", "Name:", "PCI-ID:", "Lowest:", "IB-Port:");
	next = get_next_device(&devices_list);
	while (next) {
		printf("%18.18s", next->pnetid);
		printf(" %10.10s", next->subsystem);
		printf(" %15.15s", next->name);
		if (next->parent)
			printf(" %16.16s", next->parent);
		else
			printf(" %16.16s", "n/a");
		if (next->lowest)
			printf(" %15.15s", next->lowest);
		else
			printf(" %15.15s", "n/a");
		if (next->ib_port != -1)
			printf(" %9d", next->ib_port);
		else
			printf(" %9.9s", "n/a");
		printf("\n");

		next = get_next_device(next);
	}

	/* free devices in devices list */
	free_devices();

	return 0;
}
