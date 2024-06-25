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

void strip_whitespace(char* str) {
    if (str == NULL) return;
    size_t len = strlen(str);
    if (len > 0 && str[len -1] == ' ') str[len -1] = '\0';
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
    ssize_t bytes_received = recv(sock_fd, response_buffer, max_len -1, 0);
    if (bytes_received == -1) {
        return -1;
    } else {
        response_buffer[bytes_received] = '\0';
        strip_whitespace(response_buffer);
    }
    return bytes_received;
}


void break_out_of_main_loop() {
    in_main_loop = false;
}


int main(void) {
    signal(SIGINT, break_out_of_main_loop);
    
    int sock_fd = setup_socket();
    
    char buffer[BUFFER_SIZE] = "";

    in_main_loop = true;
    while (in_main_loop) {
        // accept the client connection
        int client_fd;
        struct sockaddr_un client_address;
        socklen_t client_address_len = sizeof(client_address);
        client_fd = accept(sock_fd, (struct sockaddr *)&client_address, &client_address_len);

        // read response command
        receive_message(client_fd, buffer,BUFFER_SIZE);
        
        // act upon the command
        
        close(client_fd);
    }
    
    cleanup(sock_fd);
    
    return 0;
}



