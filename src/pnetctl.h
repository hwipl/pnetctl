#ifndef _PNETCTL_H
#define _PNETCTL_H

#define SMC_MAX_PNETID_LEN 16 /* maximum length of pnetids */
#define IB_DEFAULT_PORT 1 /* default port for infiniband devices */

void verbose(const char *format, ...);
void set_pnetid_for_eth(const char *dev_name, const char* pnetid);
void set_pnetid_for_ib(const char *dev_name, int dev_port, const char* pnetid);

#endif
