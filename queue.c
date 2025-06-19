#include <stdlib.h>
#include <string.h>
#include "queue.h"

int is_empty(Queue *q) {
    if(q->front == NULL) 
        return 1;
    return 0;
}

void init_queue(Queue *q) {
    q->front = NULL;
    q->rear = NULL;
}

void enqueue(Queue *q, char *file_name) {
    if (q == NULL || file_name == NULL)
        return;

    Node *newNode = (Node *)malloc(sizeof(Node));
    memcpy(newNode->file, file_name, FILE_NAME_LEN);
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
        return;
    }
    q->rear->next = newNode;
    q->rear = newNode;
}

void dequeue(Queue *q, char *res) {
    if (q->front == NULL) {
        res = NULL;
        return;
    }

    Node *temp = q->front;
    memcpy(res, temp->file, FILE_NAME_LEN);

    q->front = q->front->next;
    if (q->front == NULL)
        q->rear = NULL;
    free(temp);
}

void free_queue(Queue *q) {
    Node *f = q->front, *s;
    while (f != NULL) {
        s = f->next;
        free(f);
        f = s;
    }
}
