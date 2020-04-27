/*
 * ********************
 * *** NETLINK PART ***
 * ********************
 */

#include <linux/smc.h>
#include <rdma/ib_user_verbs.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>

#include "pnetctl.h"
#include "devices.h"

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
		verbose("Got netlink message with pnetid \"%s\" and eth name "
			"\"%s\".\n",
			nla_get_string(attrs[SMC_PNETID_NAME]),
			nla_get_string(attrs[SMC_PNETID_ETHNAME]));
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
		verbose("Got netlink message with pnetid \"%s\", ib name "
			"\"%s\", and ib port \"%d\".\n",
			nla_get_string(attrs[SMC_PNETID_NAME]),
			nla_get_string(attrs[SMC_PNETID_IBNAME]),
			nla_get_u8(attrs[SMC_PNETID_IBPORT]));
		set_pnetid_for_ib(nla_get_string(attrs[SMC_PNETID_IBNAME]),
				  nla_get_u8(attrs[SMC_PNETID_IBPORT]),
				  nla_get_string(attrs[SMC_PNETID_NAME]));
	}
	return NL_OK;
}

/* receive and parse netlink error messages */
int nl_parse_error(struct sockaddr_nl *nla, struct nlmsgerr *nlerr, void *arg) {
	printf("Netlink error: %s\n", strerror(-nlerr->error));
	return NL_STOP;
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

/* delete a pnetid */
void nl_del_pnetid(const char *pnet_name) {
	struct nl_msg* msg;
	int rc;

	/* construct netlink message */
	verbose("Constructing netlink message to delete pnetid \"%s\".\n",
		pnet_name);
	msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, nl_family, 0, NLM_F_REQUEST,
		    SMC_PNETID_DEL, nl_version);
	nla_put_string(msg, SMC_PNETID_NAME, pnet_name);

	/* send and free netlink message */
	verbose("Sending delete pnetid command over netlink socket.\n");
	rc = nl_send_auto(nl_sock, msg);
	if (rc < 0)
		printf("Error sending request: %d\n", rc);
	nlmsg_free(msg);

	/* check reply */
	nl_recvmsgs_default(nl_sock);
}

/* init netlink part */
void nl_init() {
	struct nl_cb *cb;

	verbose("Initializing netlink socket.\n");
	nl_sock = nl_socket_alloc();
	cb = nl_socket_get_cb(nl_sock);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl_parse_msg, NULL);
	nl_cb_err(cb, NL_CB_CUSTOM, nl_parse_error, NULL);
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
