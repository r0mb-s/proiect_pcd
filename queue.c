#include <stdlib.h>
#include <string.h>
#include "queue.h"

int is_empty(Queue *q) {
    if(q->front == NULL) 
        return 1;
    return 0;
}

void enqueue(Queue *q, char *file_name) {
    if (q == NULL || file_name == NULL)
        return;

    Node *newNode = (Node *)malloc(sizeof(Node));
    strncpy(newNode->file_name, file_name, NAME_MAX - 1);
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
    strncpy(res, temp->file_name, NAME_MAX - 1);

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
