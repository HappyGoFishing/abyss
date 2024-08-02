#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../shared/util.h"
#include "service.h"

#define SOCKET_PATH "/tmp/abyss.sock"
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LIST_SIZE 2

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
    printf("SERVICE_PATH = %s\n", SERVICE_PATH);
    printf("MAX_SERVICE_ARRAY_SIZE = %i\n", MAX_SERVICE_ARRAY_SIZE);
    printf("SOCKET_PATH = %s\n", SOCKET_PATH);
    int sockfd = setup_socket();
    if (sockfd == -1) {
        fprintf(stderr, "failed to bind socket\n");
        exit(EXIT_FAILURE);
    } else {
        printf("listening on bound socket\n");
    }
    struct ServiceArray active_services = { .size = 0 };
    
    fd_set read_fds;
    int max_fd = sockfd + 1;

    bool running = true;
    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }
        if (FD_ISSET(sockfd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int clientfd;
            if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("accept");
            } else {
                char buffer[BUFFER_SIZE] = "";
                receive_message(clientfd, buffer, BUFFER_SIZE);

                char command_list[MAX_COMMAND_LIST_SIZE][BUFFER_SIZE];

                // reset command_list to empty
                for (int i = 0; i < MAX_COMMAND_LIST_SIZE; i++) {
                    command_list[i][0] = '\0';
                }

                // split received message into array of strings
                char *token;
                char *save_ptr = buffer;
                int i = 0;
                while ((token = strtok_r(save_ptr, " ", &save_ptr))) {
                    printf("token[%i]: %s\n", i, token);
                    strcpy(command_list[i], token);
                    i++;
                }

                if (!strcmp(command_list[0], "service_start")) {
                    struct Service service = read_service_toml_file(SERVICE_PATH, command_list[1]);
                    strcpy(service.name, command_list[1]);
                    if (!service.ok) {
                        fprintf(stderr, "couldnt start service: (service data is malformed)\n");
                    } else {
                        if (find_service_index_by_name(&active_services, service.name) != -1) {
                            printf("couldnt start service: %s (service already running)\n", service.name);
                        } else {
                            printf("starting service: %s\n", service.name);
                            printf("command=%s\nargs=%s\n", service.command, service.args);
                            start_service(service);
                            if (add_service_to_array(&active_services, service) == -1) {
                            // service failed to start
                            }
                        }
                    }
                }

                if (!strcmp(command_list[0], "service_stop")) {
                    stop_service(command_list[1], &active_services);
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