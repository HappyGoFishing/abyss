#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../shared/util.h"
#include "service.h"

#define SOCKET_PATH "/tmp/abyss.sock"
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LIST_SIZE 2

#define TOML_DIR_PATH "./"

static bool running = false;

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
    } else
        printf("bound socket, listening on: %s\n", SOCKET_PATH);

    fd_set read_fds;
    int max_fd = sockfd + 1;

    char buffer[BUFFER_SIZE] = "";

    bool running = true;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1)
            break;

        int clientfd;

        if (FD_ISSET(sockfd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("accept");
            } else {
                receive_message(clientfd, buffer, BUFFER_SIZE);
                // split the command into an array of strings
                char *token;
                char *save_ptr = buffer;
                char command_list[MAX_COMMAND_LIST_SIZE][BUFFER_SIZE];
                int i = 0;
                while ((token = strtok_r(save_ptr, " ", &save_ptr))) {
                    printf("token[%i]: %s\n", i, token);
                    strcpy(command_list[i], token);
                    i++;
                }

                if (!strcmp(command_list[0], "stop")) {
                    printf("told to stop by client\n");
                    send_message(clientfd, "stopping daemon");
                    break;
                }
                if (!strcmp(command_list[0], "service_start")) {
                    struct ServiceData service_data = read_service_toml_file(TOML_DIR_PATH, command_list[1]);
                    if (!service_data.ok) {
                        fprintf(stderr, "the service could not be started");
                    } else {
                        struct ServiceInfo service_info;
                        service_info.data = service_data;
                        strcpy(command_list[1], service_info.service_name);

                        printf("starting service: %s\n", service_info.service_name);
                        printf("command=%s\nargs=%s\n", service_data.command, service_data.args);
                        start_service(service_info);
                    }
                }
            }
            close(clientfd);
        }
    }
    printf("\nunlinking %s\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
    printf("goodbye\n");
    return 0;
}

