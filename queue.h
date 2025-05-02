#ifndef QUEUE_H
#define QUEUE_H

#include <linux/limits.h>

typedef struct Node {
    char file_name[NAME_MAX + 1];
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

int is_empty(Queue *q);
void init_queue(Queue *q);
void enqueue(Queue *q, char *file_name);
void dequeue(Queue *q, char *res);
void free_queue(Queue *q);

#endif
