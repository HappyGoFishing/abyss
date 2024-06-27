#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../shared/util.h"

#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define BUFFER_SIZE 1024

static short running = 0;

int setup_socket() {
    unlink(SOCKET_PATH);
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return -1;
    }
    if (listen(sockfd, 1) == -1) {
        perror("listen");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return sockfd;
}

void signal_handler(int sig) {
    if (sig == SIGINT)
        running = 0;
}

int main(void) {
    signal(SIGINT, signal_handler);

    int sockfd = setup_socket();
    if (sockfd == -1) {
        fprintf(stderr, "failed to bind socket %s\n", SOCKET_PATH);
        exit(1);
    }

    fd_set read_fds;
    int max_fd = sockfd + 1;

    char buffer[BUFFER_SIZE] = "";

    running = 1;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1) {
            break;
        }

        int clientfd;

        if (FD_ISSET(sockfd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) ==
                -1) {
                perror("accept");
            } else {
                receive_message(clientfd, buffer, BUFFER_SIZE);
                if (!strcmp(buffer, "stop")) {
                    send_message(clientfd, "recv stop");
                    running = 0;
                }
                send_message(clientfd, "hello world\n");
            }
            close(clientfd);
        }
    }
    printf("unlinking %s.\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
    printf("goodbye\n");
    return 0;
}
