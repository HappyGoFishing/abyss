#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>

#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define CLIENT_SKIP_PRINT_CODE "!cspc"
#define BUFFER_SIZE 1024

bool in_main_loop;

void cleanup(int sock_fd) {
    printf("cleaning up\n");
    printf("closing socket\n");
    close(sock_fd);
    printf("unlinking %s\n", SOCKET_PATH);
    unlink(SOCKET_PATH);
}

int setup_socket() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
    }
    
    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) == -1)
        perror("bind");

    if (listen(sock_fd, 1) == -1)
        perror("listen");

    if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) == -1)
        perror("fcntl");

    printf("socket bound and listening on %s\n", SOCKET_PATH);
    return sock_fd;
}


void break_out_of_main_loop() {
    in_main_loop = false;
}


int main(void) {
    signal(SIGINT, break_out_of_main_loop);
    
    int sock_fd = setup_socket();
    in_main_loop = true;
    while (in_main_loop) {
        // accept the client connection
        int client_fd;
        struct sockaddr_un client_address;
        socklen_t client_address_len = sizeof(client_address);
        client_fd = accept(sock_fd, (struct sockaddr *)&client_address, &client_address_len);

        close(client_fd);
    }

    cleanup(sock_fd);
    
    return 0;
}



