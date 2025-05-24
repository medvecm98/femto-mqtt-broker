#include "log.h"

void
log_general(const char *level, const char *fmt, va_list args) {
    char *final_string;
    size_t final_string_len = 5 + 5 + 2 + strlen(fmt) + 4 + 1 + 1;
    final_string = calloc(final_string_len, sizeof (char));
    snprintf(final_string, final_string_len, "%s: %s\n", level, fmt);
    vfprintf(stderr, final_string, args);
    free(final_string);
}

void
log_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(MAGENTA "FATAL" RESET, fmt, args);
}

void
log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(RED "ERROR" RESET, fmt, args);
}

void
log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(YELLOW "WARN " RESET, fmt, args);
}

void
log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(GREEN "INFO " RESET, fmt, args);
}

void
log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(CYAN "DEBUG" RESET, fmt, args);
}

void
log_trace(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_general(BLUE "TRACE" RESET, fmt, args);
}
