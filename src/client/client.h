#pragma once
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>

#define SOCKET_PATH "/tmp/abyss.sock"
#define BUFFER_SIZE 1024

int send_socket(int sock_fd, const char* msg);
ssize_t recv_socket(int sock_fd, char* response_buffer, size_t max_len);
int setup_socket();
void log_message(int priority, const char *format, ...);
void log_crash_message(const char *format, ...);
void strip_whitespace(char* msg);
int count_substrings(const char *str);

