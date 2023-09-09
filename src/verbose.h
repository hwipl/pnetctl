#ifndef _PNETCTL_VERBOSE_H
#define _PNETCTL_VERBOSE_H

/* verbose output, disabled by default */
extern int verbose_mode;

void verbose(const char *format, ...);

#endif
