#include "queue.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/inotify.h>

#define PORT 8090
#define ADMIN_SOCKET_PATH "/tmp/pcd_admin_socket"
#define BUFFER_SIZE 1024
#define PROCESS_QUEUE_MAX_SIZE 100
#define PROCESSING_FOLDER "processing/"
#define OUTGOING_FOLDER "outgoing/"
#define MAX_NUMBER_OF_CLIENTS 100
#define MAX_NUMBER_OF_ADMINS 1

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

void* handle_client(void* client_socket) {
    int client_fd = *((int*)client_socket);
    char buffer[BUFFER_SIZE];
    
    while (1) {
        // TO DO //
    }
    
    close(client_fd);
    return NULL;
}

void *clients_function(void *arg) {
    pthread_exit(NULL);
    int server_fd, client_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t client;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error on creating client socket");
        // TO DO //
    }
    
    memset(&server_addr, 0, addr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, addr_len == -1)) {
        perror("Couldn't bind clients socket");
        // TO DO //
    }

    if (listen(server_fd, MAX_NUMBER_OF_CLIENTS) == -1) {
        perror("Couldn't listen on server socket");
        // TO DO //
    }
    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) == -1) {
            perror("Couldn't accept client fd");
            continue;
        }

        if (pthread_create(&client, NULL, handle_client, (void*)&client_fd) != 0) {
            perror("Couldn't create thread for client");
            // TO DO //
        } else {
            if(pthread_detach(client) == -1) {
                perror("Couldn't create thread for client");
                // TO DO //
            }
        }
    }

    close(server_fd);
    pthread_exit(NULL);
}

void *soap_function(void *arg) {
    pthread_exit(NULL);
    // TO DO //
}

void process_file(char *name) {
    // TO DO //
}

void *notify_function(void *arg){
    Queue *queue = (Queue *) arg;
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("Couldn't create inotify");
        // TO DO //
    }

    int watch_fd = inotify_add_watch(inotify_fd, PROCESSING_FOLDER, IN_CREATE);
    if (watch_fd == -1) {
        perror("Couldn't add watch to folder through inotify");
        // TO DO //
    }

    char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
    while (1) {
        int length = read(inotify_fd, buffer, sizeof(struct inotify_event) + NAME_MAX + 1);
        if (length <= 0) {
            // TO DO //
        }

        struct inotify_event *event = (struct inotify_event *)buffer;
        if (event->len) {
            if (event->mask & IN_CREATE) {
                enqueue(queue, event->name);
            }
        }
    }

    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);
    pthread_exit(NULL);
}

void *processing_function(void *arg) {
    Queue *queue = (Queue *) arg;
    char buffer[NAME_MAX + 1];

    while(1) {
        if(!is_empty(queue)) {
            dequeue(queue, buffer);
            // TO DO //
        }
    }
    
    pthread_exit(NULL);
}

int main() {
    pthread_t admin_thread;
    pthread_t clients_thread;
    pthread_t soap_thread;
    pthread_t processing_thread;
    pthread_t notify_processing;
    pthread_attr_t threads_attr;

    Queue queue;
    init_queue(&queue);

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
    while(1);

    return 0;
}
