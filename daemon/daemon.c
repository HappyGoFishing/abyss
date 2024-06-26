#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include "../shared/util.h"

#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define BUFFER_SIZE 1024

static bool running = false;

int setup_socket() {
    int sock_fd;
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return -1;
    }
    if (listen(sock_fd, 1) == -1) {
        perror("listen");
        return -1;
    }
    if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return sock_fd;
}

void signal_handler(int sig) {
    if (sig == SIGINT) running = false;
}

int main(void) {
    signal(SIGINT, signal_handler);

    int sockfd = setup_socket();
    
    if (sockfd == -1) {
        fprintf(stderr, "failed to bind %s\n", SOCKET_PATH);
        exit(1);
    } else {
        printf("bound socket %s\n", SOCKET_PATH);
    }
    
    fd_set read_fds;
    int max_fd = sockfd + 1;
    
    char buffer[BUFFER_SIZE] = "";

    running = true;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        int clientfd;

        if (FD_ISSET(sockfd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("accept");
            } else {
                receive_message(clientfd, buffer, BUFFER_SIZE);
                if (!strcmp(buffer, "stop")) {
                    send_message(clientfd, "recv stop");
                    running = false; 
                }
                send_message(clientfd, "hello world\n");
            }
            close(clientfd);
        }
    }
    printf("unlinking %s and closing.\n", SOCKET_PATH);
    unlink(SOCKET_PATH); 
    return 0;
}
