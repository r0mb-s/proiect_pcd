#include "queue.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/inotify.h>
#include "log.h"
#include <string.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "cyphers.h"

#define PORT 8090
#define ADMIN_SOCKET_PATH "/tmp/pcd_admin_socket"
#define BUFFER_SIZE 1024
#define PROCESS_QUEUE_MAX_SIZE 100
#define PROCESSING_FOLDER "processing/"
#define INCOMPLETE_FOLDER "incomplete/"
#define OUTGOING_FOLDER "outgoing/"
#define MAX_NUMBER_OF_CLIENTS 100
#define MAX_NUMBER_OF_ADMINS 1
#define LOG_FILE "/tmp/pcd_log_file"
#define PACKET_SIZE 4096

Logger logger;

void *admin_function(void *arg) {
    int server_fd, admin_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_un addr;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Error on creating admin socket");
        // TO DO //
    }

    unlink(ADMIN_SOCKET_PATH);

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ADMIN_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Couldn't bind admin socket");
        // TO DO //
    }

    if (listen(server_fd, 1) == -1) {
        perror("Couldn't listen on server socket");
        // TO DO //
    }

    while (1) {
        if ((admin_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("Couldn't accept admin");
            // TO DO //
        }

        // TO DO //

        close(admin_fd);
    }
}

void *clients_function(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    uuid_t clients_uuid[MAX_NUMBER_OF_CLIENTS + 1];
    int clients_fd[MAX_NUMBER_OF_CLIENTS + 1];

    for (int i = 0; i < sizeof(clients_uuid); i++) {
        uuid_generate(clients_uuid[i]);
    }

    struct pollfd fds[MAX_NUMBER_OF_CLIENTS + 1];
    int nfds = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error on creating socket");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Couldn't bind server socket");
        close(server_fd);
        pthread_exit(NULL);
    }

    if (listen(server_fd, MAX_NUMBER_OF_CLIENTS) == -1) {
        perror("Couldn't listen on server socket");
        close(server_fd);
        pthread_exit(NULL);
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(fds, nfds, -1);
        if (poll_count == -1) {
            perror("poll failed");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    addr_len = sizeof(client_addr);
                    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
                    if (client_fd == -1) {
                        perror("accept failed");
                        continue;
                    }

                    if (nfds < MAX_NUMBER_OF_CLIENTS + 1) {
                        fds[nfds].fd = client_fd;
                        fds[nfds].events = POLLIN;
                        nfds++;
                    } else {
                        printf("Too many clients\n");
                        close(client_fd);
                    }
                } else {
                    int bytes_read = read(fds[i].fd, buffer, BUFFER_SIZE - 1);
                    if (bytes_read <= 0) {
                        close(fds[i].fd);
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--;
                    } else {
                        printf("byte read: %d\n", bytes_read);
                        int first_message;
                        memcpy(&first_message, buffer, sizeof(int));

                        if (first_message == 1) {
                            printf("First message\n");
                            uuid_t job_uuid;
                            char job_uuid_string[37], client_uuid_string[37];
                            int algorithm, key_len, enc_dec;

                            memcpy(&algorithm, buffer + sizeof(int), sizeof(int));
                            memcpy(&key_len, buffer + 2 * sizeof(int), sizeof(int));
                            printf("algorithm: %d\nkey length: %d\nenc or dec: %d\n", algorithm, key_len, enc_dec);

                            char key[key_len];

                            memcpy(&key, buffer + 3 * sizeof(int), key_len);
                            printf("key: %.*s\n", key_len, key);

                            memcpy(&enc_dec, buffer + 3 * sizeof(int) + key_len, sizeof(int));
                            printf("enc_dec: %d\n", enc_dec);

                            uuid_unparse(clients_uuid[i], client_uuid_string);

                            char folder_name[sizeof(INCOMPLETE_FOLDER) + sizeof(client_uuid_string)];
                            strncpy(folder_name, INCOMPLETE_FOLDER, sizeof(INCOMPLETE_FOLDER));
                            strncpy(folder_name + sizeof(INCOMPLETE_FOLDER) - 1, client_uuid_string, sizeof(client_uuid_string));
                            folder_name[sizeof(folder_name) - 1] = '\0';

                            printf("folder name: %s\n", folder_name);

                            char file_path[sizeof(INCOMPLETE_FOLDER) + sizeof(client_uuid_string) + 1 + sizeof(job_uuid_string) + 1];

                            uuid_generate(job_uuid);
                            uuid_unparse(job_uuid, job_uuid_string);

                            // if (mkdir(folder_name, 0777) == -1) {
                            //     perror("Couldn't create client folder!");
                            // }

                            if (write(fds[i].fd, &job_uuid, sizeof(uuid_t)) == -1) {
                                perror("Couldn't write job uuid to client!");
                                // TO DO //
                            }

                            snprintf(file_path, sizeof(file_path), "%s%s_%s", INCOMPLETE_FOLDER, client_uuid_string, job_uuid_string);
                            printf("path: %s\n", file_path);

                            if ((clients_fd[i] = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
                                perror("Couldn't create job file!");
                                // TO DO //
                            }

                            ssize_t bytes_written;
                            if ((bytes_written = write(clients_fd[i], &algorithm, sizeof(int))) <= 0) {
                                perror("Couldn't write to client file!");
                                // TO DO //
                            }
                            if ((bytes_written = write(clients_fd[i], &key_len, sizeof(int))) <= 0) {
                                perror("Couldn't write to client file!");
                                // TO DO //
                            }
                            if ((bytes_written = write(clients_fd[i], &key, key_len)) <= 0) {
                                perror("Couldn't write to client file!");
                                // TO DO //
                            }
                            if ((bytes_written = write(clients_fd[i], &enc_dec, sizeof(int))) <= 0) {
                                perror("Couldn't write to client file!");
                                // TO DO //
                            }
                        } else if (first_message == 0 || first_message == 2) {
                            printf("Second message\n");
                            printf("message number: %d\n", first_message);

                            if (first_message == 2) {
                                close(clients_fd[i]);
                                printf("AM ajuns la ultimul pachet\n");

                                char incomplete_path[sizeof(INCOMPLETE_FOLDER) + FILE_NAME_LEN];
                                char process_file_path[sizeof(PROCESSING_FOLDER) + FILE_NAME_LEN];

                                uuid_t job_uuid;
                                char job_uuid_string[37], client_uuid_string[37];
                                int message_len;

                                memcpy(&job_uuid, buffer + sizeof(int), sizeof(uuid_t));

                                uuid_unparse(job_uuid, job_uuid_string);
                                uuid_unparse(clients_uuid[i], client_uuid_string);

                                snprintf(incomplete_path, sizeof(incomplete_path), "%s%s_%s", INCOMPLETE_FOLDER, client_uuid_string, job_uuid_string);
                                snprintf(process_file_path, sizeof(process_file_path), "%s%s_%s", PROCESSING_FOLDER, client_uuid_string, job_uuid_string);

                                printf("incomplete path: %s\n", incomplete_path);
                                printf("processing path: %s\n", process_file_path);

                                if (rename(incomplete_path, process_file_path) == -1) {
                                    perror("error on move");
                                    // TO DO //
                                }
                                continue;
                            }

                            uuid_t job_uuid;
                            char job_uuid_string[37], client_uuid_string[37];
                            int message_len;

                            memcpy(&job_uuid, buffer + sizeof(int), sizeof(uuid_t));
                            memcpy(&message_len, buffer + sizeof(int) + sizeof(uuid_t), sizeof(int));

                            uuid_unparse(job_uuid, job_uuid_string);
                            uuid_unparse(clients_uuid[i], client_uuid_string);

                            printf("client uuid: %s\njob_uuid: %s\n", client_uuid_string, job_uuid_string);

                            printf("message len: %d\n", message_len);
                            ssize_t bytes_written;
                            if ((bytes_written = write(clients_fd[i], buffer + sizeof(int) + sizeof(uuid_t) + sizeof(int), message_len)) <= 0) {
                                perror("Couldn't write to client file!");
                                // TO DO //
                            }
                        } else {
                            printf("Belea mare\n");
                            close(clients_fd[i]);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < nfds; i++) {
        close(fds[i].fd);
    }
    pthread_exit(NULL);
}

void *soap_function(void *arg) {
    pthread_exit(NULL);
    // TO DO //
}

void process_file(char *name) {
    // TO DO //
}

void *notify_function(void *arg) {
    Queue *queue = (Queue *)arg;
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("Couldn't create inotify");
        // TO DO //
    }

    int watch_fd = inotify_add_watch(inotify_fd, PROCESSING_FOLDER, IN_MOVED_TO);
    if (watch_fd == -1) {
        perror("Couldn't add watch to folder through inotify");
        // TO DO //
    }

    char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
    while (1) {
        int length = read(inotify_fd, buffer, sizeof(struct inotify_event) + NAME_MAX + 1);
        if (length <= 0) {
            // TO DO //
            continue;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            if (event->len > 0 && (event->mask & IN_MOVED_TO)) {
                printf("New thing created: %s\n", event->name);
                enqueue(queue, event->name);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);
    pthread_exit(NULL);
}

void *processing_function(void *arg) {
    Queue *queue = (Queue *)arg;
    char buffer[FILE_NAME_LEN];

    while (1) {
        if (!is_empty(queue)) {
            dequeue(queue, buffer);

            char file_path[sizeof(PROCESSING_FOLDER) + FILE_NAME_LEN];
            char out_path[sizeof(OUTGOING_FOLDER) + FILE_NAME_LEN];
            snprintf(file_path, sizeof(file_path), "%s%s", PROCESSING_FOLDER, buffer);
            snprintf(out_path, sizeof(out_path), "%s%s", OUTGOING_FOLDER, buffer);

            printf("file path: %s\n", file_path);

            int file_fd;
            if((file_fd = open(file_path, O_RDONLY)) == -1) {
                perror("Couldn't open file in processing function!");
                // TO DO //
                exit(EXIT_FAILURE);
            }

            ssize_t bytes_read;
            char content_buffer[BUFFER_SIZE];

            int algorithm, key_len, enc_dec;
            if((bytes_read = read(file_fd, &algorithm, sizeof(int))) <= 0){
                fprintf(stderr, "couldnt read algorithm in processing function");
            }
            if((bytes_read = read(file_fd, &key_len, sizeof(int))) <= 0){
                fprintf(stderr, "couldnt read key_len in processing function");
            }

            char key[key_len];
            if((bytes_read = read(file_fd, &key, key_len)) <= 0){
                fprintf(stderr, "couldnt read key in processing function");
            }

            if((bytes_read = read(file_fd, &enc_dec, sizeof(int))) <= 0){
                fprintf(stderr, "couldnt read enc_dec in processing function");
            }

            printf("Algorithm: %d, Key length: %d, Key: %.*s, Enc or dec: %d\n", algorithm, key_len, key_len, key, enc_dec);

            while((bytes_read = read(file_fd, content_buffer, sizeof(content_buffer))) > 0) {
                printf("buffer: %.*s\n", (int) bytes_read, content_buffer);

                run_symmetric(algorithm, enc_dec, file_path, out_path, key);
            }
        }
    }

    pthread_exit(NULL);
}

int main() {
    logger.log_file = LOG_FILE;
    log_message(&logger, "Start");

    pthread_t admin_thread;
    pthread_t clients_thread;
    pthread_t soap_thread;
    pthread_t processing_thread;
    pthread_t notify_processing;
    pthread_attr_t threads_attr;

    Queue queue;
    init_queue(&queue);
    log_message(&logger, "Initialised queue");

    pthread_attr_init(&threads_attr);
    pthread_attr_setdetachstate(&threads_attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&admin_thread, NULL, admin_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&clients_thread, NULL, clients_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&soap_thread, NULL, soap_function, NULL) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&processing_thread, NULL, processing_function, &queue) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&notify_processing, NULL, notify_function, &queue) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    pthread_attr_destroy(&threads_attr);
    while (1)
        ;

    return 0;
}
