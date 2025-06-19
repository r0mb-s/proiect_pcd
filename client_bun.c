#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <ncurses.h>

#include "packet.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8090
#define BUFFER_SIZE 1024

void collect_header_info(Header *h, int up_down);
void start_interface();
void handle_upload();
void handle_download();
void upload_file(const char *filename, Header *header);

int main()
{
    start_interface();
    return 0;
}

void start_interface()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int choice;
    while (1)
    {
        clear();
        mvprintw(2, 2, "File Transfer Client");
        mvprintw(4, 4, "1. Upload File");
        mvprintw(5, 4, "2. Download File (not implemented)");
        mvprintw(6, 4, "3. Exit");
        mvprintw(8, 2, "Select an option: ");
        refresh();

        choice = getch();
        switch (choice)
        {
        case '1':
            handle_upload();
            break;
        case '2':
            handle_download();
            break;
        case '3':
            endwin();
            exit(0);
        default:
            mvprintw(10, 2, "Invalid choice. Press any key.");
            getch();
        }
    }
}

void collect_header_info(Header *h, int up_down)
{
    echo();
    char buffer[256];

    h->up_down = up_down;

    // Algorithm
    clear();
    mvprintw(2, 2, "Select Algorithm:");
    mvprintw(4, 4, "1. AES");
    mvprintw(5, 4, "2. CHACHA20");
    mvprintw(8, 2, "Choice: ");
    refresh();
    getnstr(buffer, sizeof(buffer));
    h->algorithm = atoi(buffer);
    if (h->algorithm == 1)
    {
        h->algorithm = 0;
    }
    else
    {
        h->algorithm = 1;
    }
    // Key
    clear();
    mvprintw(2, 2, "Enter encryption/decryption key (max 254 chars): ");
    refresh();
    getnstr(buffer, sizeof(buffer));
    strncpy(h->key, buffer, sizeof(h->key) - 1);
    h->key_len = strlen(h->key);

    // Encrypt or Decrypt
    clear();
    mvprintw(2, 2, "Select Mode:");
    mvprintw(4, 4, "1. Encrypt");
    mvprintw(5, 4, "2. Decrypt");
    mvprintw(7, 2, "Choice: ");
    refresh();
    getnstr(buffer, sizeof(buffer));
    h->enc_dec = atoi(buffer);
    if (h->enc_dec == 1)
    {
        h->enc_dec = 0;
    }
    else
    {
        h->enc_dec = 1;
    }

    h->first_middle_last = 1;
    h->message_len = 0;

    noecho();
    print_header(h);
}

void handle_upload()
{
    Header h;
    init_header(&h);
    char filepath[256];

    echo();
    clear();
    mvprintw(2, 2, "Enter path to file to upload: ");
    refresh();
    getnstr(filepath, sizeof(filepath));
    noecho();

    collect_header_info(&h, 0); // 0 = upload
    upload_file(filepath, &h);

    mvprintw(10, 2, "Upload complete or failed. Press any key to return.");
    getch();
}

void handle_download()
{
    clear();
    mvprintw(2, 2, "Download not implemented yet.");
    mvprintw(4, 2, "Press any key to return.");
    getch();
}

void upload_file(const char *filename, Header *header)
{
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
    make_packet(header, buffer, BUFFER_SIZE);


    if (send(sockfd, buffer, sizeof(buffer), 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(recv(sockfd, &buffer, BUFFER_SIZE, 0) < 0){
        perror("Error on receiving packet!");
        exit(EXIT_FAILURE);
    }
    header_from_buffer(header, buffer, BUFFER_SIZE);

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Failed to open file to send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer + sizeof(Header), BUFFER_SIZE - sizeof(Header))) > 0) {
        header->message_len = bytes_read;
        header->first_middle_last = 0;
        make_packet(header, buffer, BUFFER_SIZE);

        print_header(header);
        printf("\n------------------------\n");
        printf("\nBuffer: %.*s\n\n", header->message_len, buffer + sizeof(Header));

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

    header->first_middle_last = 2;
    header->message_len = 0;
    make_packet(header, buffer, BUFFER_SIZE);

    if (send(sockfd, buffer, BUFFER_SIZE, 0) == -1) {
        perror("Send failed (first message)");
        close(sockfd);
        close(file_fd);
        exit(EXIT_FAILURE);
    }

    close(file_fd);
    close(sockfd);
}