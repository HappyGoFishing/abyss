#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "client.h"

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("usage: abyssctl <command>\n");
        return 0;
    }

    int sockfd = setup_socket();
    if (sockfd == -1) {
        fprintf(stderr, "failed to connect socket %s\n", SOCKET_PATH);
        exit(1);
    }

    char buffer[BUFFER_SIZE] = "";

    for (int i = 1; i < argc; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    };

    strip_whitespace(buffer);

    if (send_socket(sockfd, buffer) != 0)
        exit(EXIT_FAILURE);
    close(sockfd);
    return 0;
}
