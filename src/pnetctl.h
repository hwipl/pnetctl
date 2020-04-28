#ifndef _PNETCTL_H
#define _PNETCTL_H

#define SMC_MAX_PNETID_LEN 16 /* maximum length of pnetids */
#define IB_DEFAULT_PORT 1 /* default port for infiniband devices */

int parse_cmd_line(int argc, char **argv);
void verbose(const char *format, ...);

#endif
