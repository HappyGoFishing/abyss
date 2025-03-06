#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

#include "constant_defines.h"
#include "../shared/util.h"
#include "service.h"

static int running = 0;

int setup_socket() {
    unlink(SOCKET_PATH);
    int fd_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_sock == -1) {
        perror("error socket");
        return -1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(fd_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("error bind");
        return -1;
    }
    if (listen(fd_sock, 1) == -1) {
        perror("error listen");
        return -1;
    }
    if (fcntl(fd_sock, F_SETFL, O_NONBLOCK) == -1) {
        perror("error fcntl");
        return -1;
    }
    return fd_sock;
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

void start_autostart_services(struct ServiceArray* sa) {
    printf("attempting to start autostart services\n");
    
    FILE *fp = fopen(SERVICE_AUTOSTART_LIST_FILE, "rb");
    if (fp == NULL) {
        fprintf(stderr, "error: couldn't open %s\n", SERVICE_AUTOSTART_LIST_FILE);
        return;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("error fseek");
        fclose(fp);
        return;
    }

    long fsize = ftell(fp);
    
    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("error fseek");
        fclose(fp);
        return;
    }
    
    char *rdbuf = malloc(fsize);
    if (rdbuf == NULL) {
        perror("error malloc");
        fclose(fp);
        return;
    }

    fread(rdbuf, 1, fsize, fp);
    fclose(fp);
    
    char *name_token;
    char *save_ptr = rdbuf;

    while ((name_token = strtok_r(save_ptr, "\n", &save_ptr)) != NULL) {
        
        struct Service *service = read_service_toml_file(SERVICE_CONFIG_DIR_PATH, name_token);
        if (service == NULL) {
            fprintf(stderr, "error: couldn't read service config for %s\n", name_token);
            continue;
        }
        
        strcpy(service->name, name_token);

        if (find_service_index_by_name(sa, service->name) != RESULT_SERVICE_NOT_IN_ARRAY) {
            fprintf(stderr, "error: couldn't start %s service already running\n", service->name);
            free(service);
            continue;
        }

        printf("starting service: %s\n\tcommand=%s\n\targs=%s\n", service->name, service->command, service->args);
        int child_pipefds[2]; // used by child to send pid back to parent after fork
        if (pipe(child_pipefds) == -1) {
            perror("pipe");
            free(service);
            continue;
        }

        start_service(service, child_pipefds);
        if (add_service_to_array(sa, *service) == RESULT_SERVICE_ARRAY_REACHED_LIMIT) {
            fatal_panic("service array somehow at max size during autostart phase");
        }
        
        free(service);
    }
}

int main(void) {
    signal(SIGINT, signal_handler);

    int fd_sock = setup_socket();
    if (fd_sock == -1) {
        fatal_panic("failed to bind to socket");
    }
    
    printf("listening on bound socket: %s\n", SOCKET_PATH);
    
    struct pollfd fds[1];
    fds[0].fd = fd_sock;
    fds[0].events = POLLIN;

    running = 1;

    struct ServiceArray sa = { .size = 0 }; // the services currently active
    
    start_autostart_services(&sa);

    while (running) {
        int poll_ret = poll(fds, 1, -1);
        if (poll_ret == -1) {
            perror("error poll");
            break;
        }

        if (fds[0].revents & POLLIN) {

            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int fd_client;
            if ((fd_client = accept(fd_sock, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("error accept");
                continue;
            }
            
            char buffer[BUFFER_SIZE] = "";
            receive_message(fd_client, buffer, BUFFER_SIZE);
            
            char command_list[MAX_COMMAND_LIST_SIZE][BUFFER_SIZE];
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
                struct Service *service = read_service_toml_file(SERVICE_CONFIG_DIR_PATH, command_list[1]);
                if (service == NULL) {
                    fprintf(stderr, "error: couldn't read service config for %s\n", command_list[1]);
                    close(fd_client);
                    continue;
                }
                strcpy(service->name, command_list[1]);
                
                if (find_service_index_by_name(&sa, service->name) != RESULT_SERVICE_NOT_IN_ARRAY) {
                    printf("not starting service: %s is already running\n", service->name);
                    close(fd_client);
                    continue;
                } 
                printf("starting service: %s\n\tcommand=%s\n\targs=%s\n", service->name, service->command, service->args);
                int child_pipefds[2]; // used by child to send pid back to parent after fork
                if (pipe(child_pipefds) == -1) {
                    perror("error pipe");
                    close(fd_client);
                    continue;
                }
                
                start_service(service, child_pipefds);
                if (add_service_to_array(&sa, *service) == RESULT_SERVICE_ARRAY_REACHED_LIMIT) {
                    fprintf(stderr, "error: couldn't start service %s because max service number %i has been reached\n", service->name, MAX_SERVICE_ARRAY_SIZE);
                    close(fd_client);
                    continue;
                }
            }
    
            if (!strcmp(command_list[0], "service-stop")) {
                if (find_service_index_by_name(&sa, command_list[1]) == RESULT_SERVICE_NOT_IN_ARRAY) {
                    printf("couldn't stop service: %s service was not running\n", command_list[1]);
                    close(fd_client);
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
            close(fd_client);
        }
    }
    printf("\nunlinking %s\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
    printf("goodbye\n");
    return 0;
}
