#ifndef _PNETCTL_VERBOSE_H
#define _PNETCTL_VERBOSE_H

#include <stdio.h>
#include <stdarg.h>

#include "verbose.h"

/* print verbose output to the screen if in verbose mode */
void verbose(const char *format, ...) {
	va_list args;

	va_start(args, format);
	if (verbose_mode)
		vprintf(format, args);
	va_end(args);
}

#endif
