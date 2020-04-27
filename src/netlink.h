#ifndef _PNETCTL_NETLINK_H
#define _PNETCTL_NETLINK_H

void nl_init();
void nl_cleanup();
void nl_flush_pnetids();
void nl_del_pnetid(const char *pnet_name);
void nl_set_pnetid(const char *pnet_name, const char *eth_name,
		   const char *ib_name, char ib_port);
void nl_get_pnetids();

#endif
