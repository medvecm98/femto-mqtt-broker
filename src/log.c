#include "log.h"

void
log_general(const char *level, const char *fmt, va_list args) {
    char *final_string;
    size_t final_string_len = 5 + 2 + strlen(fmt) + 1 + 1;
    final_string = calloc(final_string_len, sizeof (char));
    snprintf(final_string, final_string_len, "%s: %s\n", level, fmt);
    vfprintf(stderr, final_string, args);
    free(final_string);
}

void
log_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("FATAL", fmt, args);
}

void
log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("ERROR", fmt, args);
}

void
log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("WARN ", fmt, args);
}

void
log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("INFO ", fmt, args);
}

void
log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("DEBUG", fmt, args);
}

void
log_trace(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general("TRACE", fmt, args);
}
