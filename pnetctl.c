#include <libudev.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <rdma/ib_user_verbs.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <linux/smc.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <iconv.h>

#define SMC_MAX_PNETID_LEN 16 /* maximum length of pnetids */
#define IB_DEFAULT_PORT 1 /* default port for infiniband devices */
#define DEV_TYPE_ISM "ism" /* device type for ISM devices */

/* CCW device constants */
#define CCW_CHPID_LEN 128 /* maximum chpid length */
#define CCW_UTIL_PREFIX "/sys/devices/css0/chp0." /* util string path prefix */

void set_pnetid_for_ib(const char *dev_name, int dev_port, const char* pnetid);
void set_pnetid_for_eth(const char *dev_name, const char* pnetid);

/* verbose output, disabled by default */
int verbose_mode = 0;

/* print verbose output to the screen if in verbose mode */
void verbose(const char *format, ...) {
	va_list args;

	va_start(args, format);
	if (verbose_mode)
		vprintf(format, args);
	va_end(args);
}

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
	verbose("Sending flush pnetids command over netlink socket.\n");
	genl_send_simple(nl_sock, nl_family, SMC_PNETID_FLUSH, nl_version, 0);
	nl_recvmsgs_default(nl_sock);
}

/* get all pnetids */
void nl_get_pnetids() {
	verbose("Sending get pnetids command over netlink socket.\n");
	genl_send_simple(nl_sock, nl_family, SMC_PNETID_GET, nl_version,
			 NLM_F_DUMP);
	/* check reply */
	nl_recvmsgs_default(nl_sock);
}

/* set pnetid */
void nl_set_pnetid(const char *pnet_name, const char *eth_name,
		   const char *ib_name, char ib_port) {
	struct nl_msg* msg;
	int rc;

	/* construct netlink message */
	msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, nl_family, 0, NLM_F_REQUEST,
		    SMC_PNETID_ADD, nl_version);
	nla_put_string(msg, SMC_PNETID_NAME, pnet_name);
	if (eth_name) {
		nla_put_string(msg, SMC_PNETID_ETHNAME, eth_name);
		verbose("Constructing netlink message to add pnetid \"%s\" "
			"on net device \"%s\".\n", pnet_name, eth_name);
	}
	if (ib_name) {
		nla_put_string(msg, SMC_PNETID_IBNAME, ib_name);
		if (ib_port == -1)
			ib_port = IB_DEFAULT_PORT;
		nla_put_u8(msg, SMC_PNETID_IBPORT, ib_port);
		verbose("Constructing netlink message to add pnetid \"%s\" "
			"on ib device \"%s\" and port \"%d\".\n", pnet_name,
			ib_name, ib_port);
	}

	/* send and free netlink message */
	verbose("Sending add pnetid command over netlink socket.\n");
	rc = nl_send_auto(nl_sock, msg);
	if (rc < 0)
		printf("Error sending request: %d\n", rc);
	nlmsg_free(msg);

	/* check reply */
	nl_recvmsgs_default(nl_sock);
}

/* init netlink part */
void nl_init() {
	verbose("Initializing netlink socket.\n");
	nl_sock = nl_socket_alloc();
	nl_socket_modify_cb(nl_sock, NL_CB_VALID, NL_CB_CUSTOM, nl_parse_msg,
			    NULL);
	genl_connect(nl_sock);
	nl_family = genl_ctrl_resolve(nl_sock, SMCR_GENL_FAMILY_NAME);
	nl_version = SMCR_GENL_FAMILY_VERSION;
}

/* cleanup netlink part */
void nl_cleanup() {
	verbose("Cleaning up netlink socket.\n");
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

	verbose("Freeing devices in device table.\n");
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

/*
 * *****************
 * *** MAIN PART ***
 * *****************
 */

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
			if (pnetid_found) {
				/* another device with same pnetid? */
				if (!strncmp(next->pnetid, pnetid,
					     SMC_MAX_PNETID_LEN)) {
					print_device(next);
					next->output = 1;
				}
			} else {
				/* found new pnetid in list */
				pnetid_found = 1;
				pnetid = next->pnetid;
				print_pnetid(pnetid);
				print_device(next);
				next->output = 1;
			}
			next = get_next_device(next);
		}
		if (pnetid_found)
			print_line();
	}

	/* print remaining devices */
	print_pnetid("n/a");
	next = get_next_device(&devices_list);
	while (next) {
		if (!next->output)
			print_device(next);
		next = get_next_device(next);
	}
}

/* print usage */
void print_usage() {
	printf("------------------------------------------------------------\n"
	       "Usage:\n"
	       "------------------------------------------------------------\n"
	       "pnetctl			Print all devices and pnetids\n"
	       "pnetctl <options>	Run commands specified by options\n"
	       "			(see below)\n"
	       "------------------------------------------------------------\n"
	       "Options:\n"
	       "------------------------------------------------------------\n"
	       "-a <pnetid>		Add pnetid. Requires -n or -i\n"
	       "-r <pnetid>		Remove pnetid\n"
	       "-f			Flush pnetids\n"
	       "-n <net_dev>		Specify net device\n"
	       "-i <ib_dev>		Specify infiniband device\n"
	       "-p <ib_port>		Specify infiniband port\n"
	       "			(default: %d)\n"
	       "-v			Print verbose output\n"
	       "-h			Print this help\n",
	       IB_DEFAULT_PORT
	       );
}

/* parse command line arguments and call other functions */
int parse_cmd_line(int argc, char **argv) {
	char *net_device = NULL;
	char *ib_device = NULL;
	char *pnetid = NULL;
	char ib_port = -1;
	int remove = 0;
	int flush = 0;
	int add = 0;
	int rc;
	int c;

	/* try to get all arguments */
	while ((c = getopt (argc, argv, "a:fhi:n:p:r:v")) != -1) {
		switch (c) {
		case 'a':
			add = 1;
			pnetid = optarg;
			break;
		case 'f':
			flush = 1;
			break;
		case 'r':
			remove = 1;
			pnetid = optarg;
			break;
		case 'n':
			net_device = optarg;
			break;
		case 'i':
			ib_device = optarg;
			break;
		case 'p':
			ib_port = atoi(optarg);
			break;
		case 'v':
			verbose_mode = 1;
			break;
		case 'h':
			print_usage();
			return EXIT_SUCCESS;
		default:
			goto fail;
		}
	}

	/* check for conflicting command line parameters */
	if ((add && flush) || (add && remove) || (remove && flush)) {
		verbose("Conflicting command line arguments.\n");
		goto fail;
	}

	if (flush) {
		/* flush all pnetids and quit */
		verbose("Flushing all pnetids.\n");
		nl_init();
		nl_flush_pnetids();
		nl_cleanup();
		return EXIT_SUCCESS;
	}

	if (remove) {
		/* remove a specific pnetid */
		verbose("Removing pnetid \"%s\".\n", pnetid);
		// TODO: add pnetid removing
		verbose("NOTE: removing specific pnetid not implemented!\n");
		return EXIT_SUCCESS;
	}

	if (add) {
		/* add a pnetid entry */
		verbose("Adding pnetid \"%s\".\n", pnetid);
		if (!ib_device && !net_device) {
			verbose("Missing ib or net device.\n");
			goto fail;
		}
		nl_init();
		nl_set_pnetid(pnetid, net_device, ib_device, ib_port);
		nl_cleanup();
		return EXIT_SUCCESS;
	}

	/* No special commands, print device table to screen if there was
	 * no command line argument or if we are in verbose mode
	 */
	if (argc > 1 && !verbose_mode)
		goto fail;

	/* get all devices via udev and put them in devices list */
	verbose("Trying to find devices and read their pnetids from "
		"util_strings.\n");
	rc = udev_scan_devices();
	if (rc)
		return rc;

	/* try to receive pnetids via netlink */
	verbose("Trying to read pnetids via netlink.\n");
	nl_init();
	nl_get_pnetids();
	nl_cleanup();

	/* print devices to the screen, cleanup, and exit */
	verbose("Printing device table.\n");
	print_device_table();
	free_devices();
	return EXIT_SUCCESS;
fail:
	print_usage();
	return EXIT_FAILURE;
}

/* main function */
int main(int argc, char **argv) {
	int rc;

	/* parse command line arguments and run everything else from there */
	rc = parse_cmd_line(argc, argv);

	exit(rc);
}
