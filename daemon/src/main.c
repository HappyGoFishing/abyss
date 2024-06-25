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

#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define BUFFER_SIZE 1024

#define CODE_STOP "stop"
#define CODE_CLIENT_IGNORE_MESSAGE "ignore"

static bool in_main_loop = true;

void cleanup(int sock_fd) {
    printf("cleaning up\n");
    printf("closing socket\n");
    close(sock_fd);
    printf("unlinking %s\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
}

int setup_socket() {
    int sock_fd;
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0) == -1)) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

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

    printf("socket bound and listening on %s\n", SOCKET_PATH);
    return sock_fd;
}

void strip_whitespace(char* str) {
    if (str == NULL) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }
}

int send_message(int sock_fd, const char* msg) {
    ssize_t msg_size = strlen(msg);
    ssize_t sent_bytes = send(sock_fd, msg, msg_size, 0);
    if (sent_bytes == -1) {
        return -1;
    }
    return 0;
}

ssize_t receive_message(int sock_fd, char* response_buffer, size_t max_len) {
    ssize_t bytes_received = recv(sock_fd, response_buffer, max_len - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        return -1;
    } else {
        response_buffer[bytes_received] = '\0';
        strip_whitespace(response_buffer);
    }
    return bytes_received;
}

void signal_handler(int sig) {
    if (sig == SIGINT) in_main_loop = false;
}

int main(void) {
    signal(SIGINT, signal_handler);
    
    int sock_fd;
    if ((sock_fd =  setup_socket() == -1)) {
        fprintf(stderr, "failed to bind %s\n", SOCKET_PATH);
        cleanup(sock_fd);
        exit(1);
    }

    fd_set read_fds;
    int max_fd = sock_fd + 1;
    
    char buffer[BUFFER_SIZE] = "";

    while (in_main_loop) {
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);

        if (select(max_fd, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }
        int client_fd;
        if (FD_ISSET(sock_fd, &read_fds)) {
            // wait for client connection
            struct sockaddr_un client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                perror("accept");
            } else {
                // read client message to string buffer
                if (receive_message(client_fd, buffer, BUFFER_SIZE) == -1) {
                    perror("receive_message");
                } else {
                    // process the command
                    if (!strcmp(buffer, CODE_STOP)) {
                        send_message(client_fd, "closed daemon");
                        in_main_loop = false; // told to shutdown by client.
                    }
                }
                close(client_fd);
            }
        }
    }
    
    cleanup(sock_fd);
    
    return 0;
}
