#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
#include "daemon.h"

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

int send_socket(int sock_fd, const char *msg) {
    ssize_t msg_size = strlen(msg);
    ssize_t sent_bytes = send(sock_fd, msg, msg_size, 0);
    if (sent_bytes == -1) {
        log_message(LOG_ERR, "recv send %s", strerror(errno));
        return -1;
    }
    return 0;
}

ssize_t recv_socket(int sock_fd, char *response_buffer, size_t max_len) {
    ssize_t bytes_received = recv(sock_fd, response_buffer, max_len - 1, 0);
    if (bytes_received == -1) {
        log_message(LOG_ERR, "recv error %s", strerror(errno));
        return -1;
    } else {
        response_buffer[bytes_received] = '\0';
        strip_whitespace(response_buffer);
    }
    return bytes_received;
}
