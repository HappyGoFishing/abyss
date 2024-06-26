#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../shared/util.h"
#define SOCKET_PATH "/tmp/abyss_watcher.sock"
#define BUFFER_SIZE 1024

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

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        return -1;
    }
    return sock_fd;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("usage: abyssctl <command>\n");
        exit(1);
    }
    
    int sockfd = setup_socket();
    
    if (sockfd == -1) {
        fprintf(stderr, "failed to connect %s\n", SOCKET_PATH);
        exit(1);
    }

    char buffer[BUFFER_SIZE] = "";
    
    for (int i = 1; i < argc; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    };

    strip_whitespace(buffer);
    
    if (send_message(sockfd, buffer) != 0) {
        exit(1);
    }

    if (receive_message(sockfd, buffer, BUFFER_SIZE) == -1) {
        exit(1);
    }

    printf("%s\n",buffer);
    
    close(sockfd);
    return 0;
}