#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/un.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include <stddef.h>
#include "daemon.h"

static int running = 0;

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
    log_message(LOG_INFO, "attempting to start autostart services");
    
    FILE *fp = fopen(SERVICE_AUTOSTART_LIST_FILE, "rb");
    if (fp == NULL) {
        log_message(LOG_ERR, "error: couldn't open %s", SERVICE_AUTOSTART_LIST_FILE);
        return;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0) {
        log_message(LOG_ERR, "error: fseek %s", strerror(errno));
        fclose(fp);
        return;
    }

    long fsize = ftell(fp);
    
    if (fseek(fp, 0, SEEK_SET) != 0) {
        log_message(LOG_ERR, "error: fseek %s", strerror(errno));
        fclose(fp);
        return;
    }
    
    char *rdbuf = malloc(fsize + 1);
    if (rdbuf == NULL) {
        log_message(LOG_ERR, "error: malloc %s", strerror(errno));
        fclose(fp);
        return;
    }

    if (fread(rdbuf, 1, fsize, fp) != (size_t) fsize) {
        log_message(LOG_ERR, "error: fread %s (%s)", strerror(errno), SERVICE_AUTOSTART_LIST_FILE);
        free(rdbuf);
        fclose(fp);
        return;
    }
    fclose(fp);

    rdbuf[fsize] = '\0';
    char *name_token;
    char *save_ptr = rdbuf;

    while ((name_token = strtok_r(save_ptr, "\n", &save_ptr)) != NULL) {
        
        struct Service *service = read_service_toml_file(SERVICES_DIR_PATH, name_token);
        if (service == NULL) {
            log_message(LOG_ERR, "error: couldn't read service config for %s", name_token);
            continue;
        }
        
        strcpy(service->name, name_token);

        if (find_service_index_by_name(sa, service->name) != RESULT_SERVICE_NOT_IN_ARRAY) {
            log_message(LOG_ERR, "error: couldn't start %s service already running", service->name);
            free(service);
            continue;
        }

        log_message(LOG_INFO, "starting service: %s command=%s args=%s", service->name, service->command, service->args);
        int child_pipefds[2]; // used by child to send pid back to parent after fork
        if (pipe(child_pipefds) == -1) {
            log_message(LOG_ERR, "error: pipe %s", strerror(errno));
            free(service);
            continue;
        }

        start_service(service, child_pipefds);
        if (add_service_to_array(sa, *service) == RESULT_SERVICE_ARRAY_REACHED_LIMIT) {
            log_crash_message("service array somehow at max size during autostart phase");
        }
        
        free(service);
    }
}

int main(void) {
    signal(SIGINT, signal_handler);
    
    openlog("abyssd", LOG_PID | LOG_CONS, LOG_DAEMON);
    
    int fd_sock = setup_socket();
    if (fd_sock == -1) {
        log_crash_message("failed to bind to socket %s", SOCKET_PATH);
    }
    
    log_message(LOG_INFO, "abyssd started, listening on bound socket: %s", SOCKET_PATH);
    
    struct pollfd fds[1];
    fds[0].fd = fd_sock;
    fds[0].events = POLLIN;

    running = 1;

    struct ServiceArray sa = { .size = 0 }; // the services currently active
    
    start_autostart_services(&sa);

    while (running) {
        int poll_ret = poll(fds, 1, -1);
        if (poll_ret == -1) {
            log_message(LOG_ERR, "error: poll %s", strerror(errno));
            break;
        }

        if (fds[0].revents & POLLIN) {

            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            int fd_client = accept(fd_sock, (struct sockaddr *)&client_addr, &client_addr_len); 
            if (fd_client == -1) {
                log_message(LOG_ERR, "error: accept %s", strerror(errno));
                continue;
            }
            
            char buffer[BUFFER_SIZE] = "";
            recv_socket(fd_client, buffer, BUFFER_SIZE);
            
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
                struct Service *service = read_service_toml_file(SERVICES_DIR_PATH, command_list[1]);
                if (service == NULL) {
                    log_message(LOG_ERR, "error: couldn't read service config for %s", command_list[1]);
                    close(fd_client);
                    continue;
                }
                strcpy(service->name, command_list[1]);
                
                if (find_service_index_by_name(&sa, service->name) != RESULT_SERVICE_NOT_IN_ARRAY) {
                    log_message(LOG_INFO, "not starting service: %s is already running", service->name);
                    close(fd_client);
                    continue;
                } 
                log_message(LOG_INFO, "starting service: %s\n\tcommand=%s\n\targs=%s", service->name, service->command, service->args);
                int child_pipefds[2]; // used by child to send pid back to parent after fork
                if (pipe(child_pipefds) == -1) {
                    log_message(LOG_ERR, "error: pipe %s", strerror(errno));
                    close(fd_client);
                    continue;
                }
                
                start_service(service, child_pipefds);
                if (add_service_to_array(&sa, *service) == RESULT_SERVICE_ARRAY_REACHED_LIMIT) {
                    log_message(LOG_ERR, "error: couldn't start service %s because max service number %i has been reached", service->name, MAX_SERVICE_ARRAY_SIZE);
                    close(fd_client);
                    continue;
                }
            }
    
            if (!strcmp(command_list[0], "service-stop")) {
                if (find_service_index_by_name(&sa, command_list[1]) == RESULT_SERVICE_NOT_IN_ARRAY) {
                    log_message(LOG_INFO, "couldn't stop service: %s service was not running", command_list[1]);
                    close(fd_client);
                    continue;
                }
                stop_service(command_list[1], &sa);
                remove_service_from_array(&sa, command_list[1]);
            }
            // this is just for debugging there will be a proper implementation that communicates with abyssctl eventually.
            if (!strcmp(command_list[0], "service-list-running")) {
                log_message(LOG_INFO, "active services (%zu): \n", sa.size);
                for (size_t i = 0; i < sa.size; i++) {
                    log_message(LOG_INFO, "%li: %s\n", i + 1,  sa.array[i].name);
                }
            }
            
            if (!strcmp(command_list[0], "test-daemon-to-client-msg")) {
                dprintf(fd_client, "hello world from daemon!\n");
            }
            close(fd_client);
        }
    }
    
    log_message(LOG_INFO, "abyssd stopping, goodbye. (unlinking %s)", SOCKET_PATH);
    unlink(SOCKET_PATH);
    closelog();
    
    return 0;
}
