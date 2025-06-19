// admin.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "shared_poll.h"

#define ADMIN_SOCKET_PATH "/tmp/pcd_admin_socket"
#define BUFFER_SIZE 1024

extern SharedPoll poll_data;

int main() {
    int sockfd;
    struct sockaddr_un addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Client: socket error");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ADMIN_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Client: connect error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Client: connected to server at %s\n", ADMIN_SOCKET_PATH);

    close(sockfd);

    return 0;
}
