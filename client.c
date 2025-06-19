#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "packet.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8090
#define BUFFER_SIZE 1024
// #define FILE_TO_SEND "lorem.txt"
#define FILE_TO_SEND "outgoing/e61d46bc-3e01-4f9c-821c-f06728c84da9_5606b4e8-49c5-46cf-9d88-6753646aba47"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    Header header;
    modify_header_no_uuid(&header, 0, 1, 0, 0, 8, "aaaaaaa\0", 1);
    make_packet(&header, buffer, BUFFER_SIZE);


    if (send(sockfd, buffer, sizeof(buffer), 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(recv(sockfd, &buffer, BUFFER_SIZE, 0) < 0){
        perror("Error on receiving packet!");
        exit(EXIT_FAILURE);
    }
    header_from_buffer(&header, buffer, BUFFER_SIZE);

    int file_fd = open(FILE_TO_SEND, O_RDONLY);
    if (file_fd == -1) {
        perror("Failed to open file to send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer + sizeof(Header), BUFFER_SIZE - sizeof(Header))) > 0) {
        header.message_len = bytes_read;
        header.first_middle_last = 0;
        make_packet(&header, buffer, BUFFER_SIZE);

        print_header(&header);
        printf("\n------------------------\n");
        printf("\nBuffer: %.*s\n\n", header.message_len, buffer + sizeof(Header));

        if (send(sockfd, buffer, BUFFER_SIZE, 0) == -1) {
            perror("Send failed (second message with file)");
            close(file_fd);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        usleep(10000);
    }

    if (bytes_read == -1) {
        perror("Read file failed");
        exit(EXIT_FAILURE);
    }

    header.first_middle_last = 2;
    header.message_len = 0;
    make_packet(&header, buffer, BUFFER_SIZE);

    if (send(sockfd, buffer, BUFFER_SIZE, 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        close(file_fd);
        exit(EXIT_FAILURE);
    }

    print_header(&header);

    close(file_fd);
    close(sockfd);
    return 0;
}