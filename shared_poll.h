#ifndef SHARED_POLL_H
#define SHARED_POLL_H

#include <poll.h>
#include <pthread.h>

#define MAX_CLIENTS 100

typedef struct {
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds;
    pthread_mutex_t lock;
} SharedPoll;

#endif