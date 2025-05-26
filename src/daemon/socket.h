#pragma once

#include <sys/types.h>

int setup_socket(void);
int send_socket(int sock_fd, const char *msg);
ssize_t recv_socket(int sock_fd, char *response_buffer, size_t max_len);
