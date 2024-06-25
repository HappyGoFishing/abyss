#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define CLIENT_SKIP_PRINT_CODE "!cspc"
#define BUFFER_SIZE 1024


int setup_socket() {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "failed to create socket\n");
        exit(1);
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));   
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "error: failed to connect to %s\n", SOCKET_PATH);
        exit(1);
    }
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

void cleanup_and_shutdown(int sock_fd, int exit_code) {
    close(sock_fd);
    exit(exit_code);
}


int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("usage: abyssctl <command>\n");
        exit(0);
    }
    int sock_fd = setup_socket();

    char buffer[BUFFER_SIZE] = ""; // used for sending and then receiving
    
    // merge program args into a single string
    for (int i = 1; i < argc; i++) {
        strncat(buffer, argv[i], BUFFER_SIZE - strlen(buffer) - 1);
        strncat(buffer, " ", BUFFER_SIZE - strlen(buffer) - 1);
    };
    strip_whitespace(buffer);
    
    // send the command to the daemon
    if (send_message(sock_fd, buffer) != 0)
        cleanup_and_shutdown(sock_fd, 1);

    
    // wait for the response
    if (receive_message(sock_fd, buffer, BUFFER_SIZE) == -1)
        cleanup_and_shutdown(sock_fd, 1);
    

    // print response
    printf("%s\n", buffer);
    cleanup_and_shutdown(sock_fd, 0);
    return 0;
}