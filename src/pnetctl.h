#ifndef _PNETCTL_H
#define _PNETCTL_H

#define SMC_MAX_PNETID_LEN 16 /* maximum length of pnetids */
#define IB_DEFAULT_PORT 1 /* default port for infiniband devices */

/* pnetid filter when printing the device table */
const char *pnetid_filter;

int parse_cmd_line(int argc, char **argv);

#endif
