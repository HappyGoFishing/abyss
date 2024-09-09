#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
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
    switch (sig) {
        case SIGINT:
            running = 0;
            break;
        case SIGTERM:
            running = 0;
            break;
    }
}

int main(void) {
    signal(SIGINT, signal_handler);
    int sockfd = setup_socket();
    if (sockfd == -1) {
        fprintf(stderr, "failed to bind socket\n");
        exit(EXIT_FAILURE);
    } else {
        printf("listening on bound socket: %s\n", SOCKET_PATH);
    }
    struct ServiceArray sa = { .size = 0 }; // the services currently active
    
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    bool running = true;

    while (running) {
        int poll_ret = poll(fds, 1, -1);
        if (poll_ret == -1) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {

            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int clientfd;
            if ((clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("accept");
                continue;
            }
            char buffer[BUFFER_SIZE] = "";
            receive_message(clientfd, buffer, BUFFER_SIZE);
            char command_list[MAX_COMMAND_LIST_SIZE][BUFFER_SIZE];
            // reset command_list to empty
            for (int i = 0; i < MAX_COMMAND_LIST_SIZE; i++) {
                command_list[i][0] = '\0';
            }
            // split received message into array of strings
            char *cmd_token;
            char *save_ptr = buffer;
            for (int i = 0; (cmd_token = strtok_r(save_ptr, " ", &save_ptr)); i++) {
                strncpy(command_list[i], cmd_token, sizeof(command_list[i]));
            }
            
            if (!strcmp(command_list[0], "service-start")) {
                struct Service *service = read_service_toml_file(SERVICE_PATH, command_list[1]);
                if (service == NULL) {
                    fprintf(stderr, "error: (couldn't read service file for %s)\n", command_list[1]);
                    close(clientfd);
                    continue;
                }
                strcpy(service->name, command_list[1]);
                
                if (find_service_index_by_name(&sa, service->name) != -1) {
                    printf("not starting service: (%s is already running)\n", service->name);
                    close(clientfd);
                    continue;
                } 
                printf("starting service: %s\n\tcommand=%s\n\targs=%s\n", service->name, service->command, service->args);
                int child_pipefds[2]; // used by child to send pid back to parent after fork
                if (pipe(child_pipefds) == -1) {
                    perror("pipe");
                    close(clientfd);
                    continue;
                }
                
                start_service(service, child_pipefds);
                if (add_service_to_array(&sa, *service) == -2) {
                    printf("couldn't start service %s (reached max service number %i)\n", service->name, MAX_SERVICE_ARRAY_SIZE);
                    close(clientfd);
                    continue;
                }
            }
    
            if (!strcmp(command_list[0], "service-stop")) {
                if (find_service_index_by_name(&sa, command_list[1]) == -1) {
                    printf("couldn't stop service: %s (service not running)\n", command_list[1]);
                    close(clientfd);
                    continue;
                }
                stop_service(command_list[1], &sa);
                remove_service_from_array(&sa, command_list[1]);
            }
            if (!strcmp(command_list[0], "service-list-running")) {
                printf("active services (%zu): \n", sa.size);
                for (size_t i = 0; i < sa.size; i++) {
                    printf("%li: %s\n", i + 1,  sa.array[i].name);
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
