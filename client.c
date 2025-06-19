#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8090
#define BUFFER_SIZE 1023
#define FILE_TO_SEND "lorem.txt"
// #define FILE_TO_SEND "random.txt"

void show_buff_int_uuid(char *buf, char *user_msg) {
    int msg, msg_len;
    uuid_t msg_u;
    memcpy(&msg, buf, sizeof(int));
    memcpy(&msg_u, buf + sizeof(int), sizeof(uuid_t));
    memcpy(&msg_len, buf + sizeof(int) + sizeof(uuid_t), sizeof(int));
    char msg_s[37];
    uuid_unparse(msg_u, msg_s);

    printf("%s - %d %s (sent %d)\n", user_msg, msg, msg_s, msg_len);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    // Set up address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // FIRST MESSAGE
    memset(buffer, 0, sizeof(buffer));
    int message_no = 1;
    int algorithm = 0;
    int key_len = 8;
    int enc_dec = 0;
    char key[key_len];
    for(int i = 0; i < key_len - 1; i++){
        key[i] = 'a';
    }
    key[key_len - 1] = '\0';
    memcpy(buffer, &message_no, sizeof(int));
    memcpy(buffer + sizeof(int), &algorithm, sizeof(int));
    memcpy(buffer + 2 * sizeof(int), &key_len, sizeof(int));
    memcpy(buffer + 3 * sizeof(int), &key, key_len);
    memcpy(buffer + 3 * sizeof(int) + key_len, &enc_dec, sizeof(int));

    if (send(sockfd, buffer, 4 * sizeof(int) + key_len, 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // RECEIVE UUID
    uuid_t job_uuid;
    char job_uuid_string[37];
    int bytes_received = recv(sockfd, &job_uuid, sizeof(uuid_t), 0);
    if (bytes_received > 0) {
        uuid_unparse(job_uuid, job_uuid_string);
        printf("Received job uuid from server: %s\n", job_uuid_string);
    } else {
        printf("No UUID received.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    show_buff_int_uuid(buffer, "Initial");

    // SECOND MESSAGE
    message_no = 0;
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &message_no, sizeof(int));
    memcpy(buffer + sizeof(int), &job_uuid, sizeof(uuid_t));

    // Read file
    int file_fd = open(FILE_TO_SEND, O_RDONLY);
    if (file_fd == -1) {
        perror("Failed to open file to send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    ssize_t offset = sizeof(int) + sizeof(uuid_t) + sizeof(int); // where to start writing file content
    while ((bytes_read = read(file_fd, buffer + offset, BUFFER_SIZE - offset)) > 0) {
        ssize_t total_to_send = offset + bytes_read;
        memcpy(buffer + sizeof(int) + sizeof(uuid_t), &bytes_read, sizeof(int));
        show_buff_int_uuid(buffer, "Iteration");

        if (send(sockfd, buffer, total_to_send, 0) == -1) {
            perror("Send failed (second message with file)");
            close(file_fd);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        usleep(10000);
    }

    if (bytes_read == -1) {
        perror("Read file failed");
    }

    memset(buffer, 0, sizeof(buffer));
    message_no = 2;
    memcpy(buffer, &message_no, sizeof(int));
    memcpy(buffer + sizeof(int), &job_uuid, sizeof(uuid_t));

    show_buff_int_uuid(buffer, "End");

    if (send(sockfd, buffer, sizeof(int) + sizeof(uuid_t), 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(file_fd);
    close(sockfd);
    return 0;
}