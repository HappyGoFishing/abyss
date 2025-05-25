#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include "client.h"

int setup_socket() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        return -1;
    }
    return sockfd;
}

int send_socket(int sock_fd, const char *msg) {
    ssize_t msg_size = strlen(msg);
    ssize_t sent_bytes = send(sock_fd, msg, msg_size, 0);
    if (sent_bytes == -1) {
        return -1;
    }
    return 0;
}

ssize_t recv_socket(int sock_fd, char *response_buffer, size_t max_len) {
    ssize_t bytes_received = recv(sock_fd, response_buffer, max_len - 1, 0);
    if (bytes_received == -1) {
        return -1;
    } else {
        response_buffer[bytes_received] = '\0';
        strip_whitespace(response_buffer);
    }
    return bytes_received;
}
