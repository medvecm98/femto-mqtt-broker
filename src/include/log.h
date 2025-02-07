#define _GNU_SOURCE

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void
log_fatal(const char *fmt, ...);

void
log_error(const char *fmt, ...);

void
log_warn(const char *fmt, ...);

void
log_info(const char *fmt, ...);

void
log_debug(const char *fmt, ...);

void
log_trace(const char *fmt, ...);


#endif
