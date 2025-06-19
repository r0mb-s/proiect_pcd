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
#include "packet.h"
#include "shared_poll.h"
#include <fnmatch.h>
#include <dirent.h>

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
SharedPoll poll_data = {
    .nfds = 1,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

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

        printf("found admin client\n");

        pthread_mutex_lock(&poll_data.lock);
        for (int i = 1; i < poll_data.nfds; i++) {
            printf("Client FD: %d\n", poll_data.fds[i].fd);
        }
        pthread_mutex_unlock(&poll_data.lock);

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

    pthread_mutex_lock(&poll_data.lock);
    poll_data.fds[0].fd = server_fd;
    poll_data.fds[0].events = POLLIN;
    pthread_mutex_unlock(&poll_data.lock);

    while (1) {
        int poll_count = poll(poll_data.fds, poll_data.nfds, -1);
        if (poll_count == -1) {
            perror("poll failed");
            break;
        }

        for (int i = 0; i < poll_data.nfds; i++) {
            if (poll_data.fds[i].revents & POLLIN) {
                if (poll_data.fds[i].fd == server_fd) {
                    addr_len = sizeof(client_addr);
                    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
                    if (client_fd == -1) {
                        perror("accept failed");
                        continue;
                    }

                    if (poll_data.nfds < MAX_NUMBER_OF_CLIENTS + 1) {
                        poll_data.fds[poll_data.nfds].fd = client_fd;
                        poll_data.fds[poll_data.nfds].events = POLLIN;
                        poll_data.nfds++;
                    } else {
                        printf("Too many clients\n");
                        close(client_fd);
                    }
                } else {
                    int bytes_read = read(poll_data.fds[i].fd, buffer, BUFFER_SIZE);
                    if (bytes_read <= 0) {
                        printf("BELEA FOARTE MARE\n");
                        close(poll_data.fds[i].fd);

                        char files_to_be_removed[37 + 1 + 1];
                        char client_to_be_removed[37];
                        uuid_unparse(clients_uuid[i], client_to_be_removed);
                        snprintf(files_to_be_removed, sizeof(files_to_be_removed), "%s*", client_to_be_removed);

                        printf("files to be removed: %s\n", files_to_be_removed);

                        DIR *dir = opendir(OUTGOING_FOLDER);
                        if (!dir) {
                            perror("opendir");
                            exit(EXIT_FAILURE);
                        }

                        struct dirent *entry;
                        while ((entry = readdir(dir)) != NULL) {
                            if (fnmatch(files_to_be_removed, entry->d_name, 0) == 0) {
                                char fname[sizeof(OUTGOING_FOLDER) + 37 + 1 + 37 + 1];
                                snprintf(fname, sizeof(fname), "%s%s", OUTGOING_FOLDER, entry->d_name);
                                printf("file: %s\n", fname);
                                if (unlink(fname) == 0) {
                                    printf("Deleted: %s\n", entry->d_name);
                                } else {
                                    perror("Error deleting file");
                                }
                            }
                        }

                        closedir(dir);

                        memcpy(clients_uuid[i], clients_uuid[poll_data.nfds - 1], sizeof(uuid_t));
                        clients_fd[i] = clients_fd[poll_data.nfds - 1];
                        poll_data.fds[i] = poll_data.fds[poll_data.nfds - 1];
                        poll_data.nfds--;
                        i--;
                    } else {
                        Header header;
                        init_header(&header);
                        header_from_buffer(&header, buffer, BUFFER_SIZE);
                        print_header(&header);
                        if (header.up_down == 0) {
                            if (header.first_middle_last == 1) {
                                printf("First message\n");
                                uuid_t job_uuid;
                                char job_uuid_string[37], client_uuid_string[37];

                                uuid_generate(job_uuid);
                                memcpy(header.user_uuid, clients_uuid[i], sizeof(uuid_t));
                                memcpy(header.job_uuid, job_uuid, sizeof(uuid_t));

                                uuid_unparse(header.job_uuid, job_uuid_string);
                                uuid_unparse(header.user_uuid, client_uuid_string);

                                char file_path[sizeof(INCOMPLETE_FOLDER) + sizeof(client_uuid_string) + 1 + sizeof(job_uuid_string) + 1];
                                make_packet(&header, buffer, BUFFER_SIZE);

                                if (write(poll_data.fds[i].fd, buffer, BUFFER_SIZE) == -1) {
                                    perror("Couldn't write job uuid to client!");
                                    // TO DO //
                                }

                                snprintf(file_path, sizeof(file_path), "%s%s_%s", INCOMPLETE_FOLDER, client_uuid_string, job_uuid_string);

                                if ((clients_fd[i] = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
                                    perror("Couldn't create job file!");
                                    // TO DO //
                                }

                                ssize_t bytes_written;
                                if ((bytes_written = write(clients_fd[i], &header, sizeof(Header))) <= 0) {
                                    perror("Couldn't write to client file!");
                                    // TO DO //
                                }
                            } else if (header.first_middle_last == 0) {
                                printf("Middle message\n");
                                ssize_t bytes_written;
                                if ((bytes_written = write(clients_fd[i], buffer + sizeof(Header), header.message_len)) <= 0) {
                                    perror("Couldn't write to client file!");
                                    exit(EXIT_FAILURE);
                                }
                            } else if (header.first_middle_last == 2) {
                                printf("AM ajuns la ultimul pachet\n");

                                close(clients_fd[i]);

                                char incomplete_path[sizeof(INCOMPLETE_FOLDER) + FILE_NAME_LEN];
                                char process_file_path[sizeof(PROCESSING_FOLDER) + FILE_NAME_LEN];

                                char job_uuid_string[37], client_uuid_string[37];

                                uuid_unparse(header.job_uuid, job_uuid_string);
                                uuid_unparse(header.user_uuid, client_uuid_string);

                                snprintf(incomplete_path, sizeof(incomplete_path), "%s%s_%s", INCOMPLETE_FOLDER, client_uuid_string, job_uuid_string);
                                snprintf(process_file_path, sizeof(process_file_path), "%s%s_%s", PROCESSING_FOLDER, client_uuid_string, job_uuid_string);

                                printf("incomplete path: %s\n", incomplete_path);
                                printf("processing path: %s\n", process_file_path);

                                if (rename(incomplete_path, process_file_path) == -1) {
                                    perror("error on move");
                                    exit(EXIT_FAILURE);
                                }
                            } else {
                                exit(EXIT_FAILURE);
                            }
                        } else {
                            if (header.first_middle_last == 1) {
                                uuid_t download_uuid;
                                char user_uuid_string[37], download_uuid_string[37];
                                uuid_generate(download_uuid);

                                uuid_unparse(download_uuid, download_uuid_string);
                                uuid_unparse(header.user_uuid, user_uuid_string);

                                char file_path[sizeof(PROCESSING_FOLDER) + sizeof(user_uuid_string) + 1 + sizeof(download_uuid_string) + 1];
                                snprintf(file_path, sizeof(file_path), "%s%s_%s", PROCESSING_FOLDER, user_uuid_string, download_uuid_string);

                                printf("file path: %s\n", file_path);

                                if ((clients_fd[i] = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
                                    perror("Couldn't create job file!");
                                    exit(EXIT_FAILURE);
                                }

                                char download_buffer[BUFFER_SIZE];
                                header.client_socket = poll_data.fds[i].fd;
                                make_packet(&header, download_buffer, BUFFER_SIZE);

                                if (write(clients_fd[i], download_buffer, BUFFER_SIZE) <= 0) {
                                    fprintf(stderr, "Couldn't write download job file");
                                    exit(EXIT_FAILURE);
                                }

                                close(clients_fd[i]);
                            }
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < poll_data.nfds; i++) {
        close(poll_data.fds[i].fd);
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

    int watch_fd = inotify_add_watch(inotify_fd, PROCESSING_FOLDER, IN_MOVED_TO | IN_CREATE);
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

            if (event->len > 0) {
                if (event->mask & IN_MOVED_TO) {
                    printf("New thing created: %s\n", event->name);
                    enqueue(queue, event->name);
                } else if (event->mask & IN_CREATE) {
                    printf("New thing created: %s\n", event->name);
                    enqueue(queue, event->name);
                }
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

            Header header;
            init_header(&header);

            char file_path[sizeof(PROCESSING_FOLDER) + FILE_NAME_LEN];
            snprintf(file_path, sizeof(file_path), "%s%s", PROCESSING_FOLDER, buffer);

            printf("file path2: %s\n", file_path);
            sleep(1);

            int file_fd;
            if ((file_fd = open(file_path, O_RDONLY)) == -1) {
                perror("Couldn't open file in processing function!");
                exit(EXIT_FAILURE);
            }

            ssize_t bytes_read;
            char content_buffer[BUFFER_SIZE];

            if ((bytes_read = read(file_fd, &content_buffer, BUFFER_SIZE)) == 0) {
                fprintf(stderr, "Empty file");
            } else if (bytes_read == -1) {
                perror("Couldn't read from designated file");
            }
            close(file_fd);

            header_from_buffer(&header, content_buffer, BUFFER_SIZE);
            print_header(&header);

            if (header.up_down == 0) {
                char out_path[sizeof(OUTGOING_FOLDER) + FILE_NAME_LEN];
                snprintf(out_path, sizeof(out_path), "%s%s", OUTGOING_FOLDER, buffer);
                run_symmetric(header.algorithm, header.enc_dec, file_path, out_path, header.key);
            } else {
                char download_path[sizeof(OUTGOING_FOLDER) + FILE_NAME_LEN];

                char unparsed_user_uuid[37], unparsed_job_uuid[37];
                uuid_unparse(header.user_uuid, unparsed_user_uuid);
                uuid_unparse(header.job_uuid, unparsed_job_uuid);

                snprintf(download_path, sizeof(download_path), "%s%s_%s", OUTGOING_FOLDER, unparsed_user_uuid, unparsed_job_uuid);
                printf("Download from %s\n", download_path);

                int download_fd;
                if ((download_fd = open(download_path, O_RDONLY)) < 0) {
                    perror("Couldn't open file to send");
                    exit(EXIT_FAILURE);
                }

                ssize_t bytes_read;
                char send_buffer[BUFFER_SIZE];
                while ((bytes_read = read(download_fd, send_buffer, BUFFER_SIZE)) > 0) {
                    if (write(header.client_socket, send_buffer, bytes_read) == -1) {
                        perror("Couldn't write job uuid to client!");
                        exit(EXIT_FAILURE);
                    }
                }

                if (bytes_read == -1) {
                    perror("Couldn't read from send file");
                    exit(EXIT_FAILURE);
                }

                close(download_fd);
            }

            if (remove(file_path) != 0) {
                perror("Couldn't remove file from processing");
                exit(EXIT_FAILURE);
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
