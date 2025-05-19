#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
#include "client.h"

void log_message(int priority, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(priority, format, args);

#ifdef DEBUG
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif

    va_end(args);
}


void log_crash_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(LOG_CRIT, format, args);

#ifdef DEBUG
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
#endif

    va_end(args);
    exit(1);
}


int count_substrings(const char *str) {
    int count = 0;
    const char *p = str;
    while(*p) {
        while (*p  == ' ') p++;
        if (*p) {
            count++;
        }
        while (*p && *p != ' ') {
            p++;
        }
    }
    return count;
}

void strip_whitespace(char *str) {
    if (str == NULL)
        return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}
