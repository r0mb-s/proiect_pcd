#include "log.h"
#include <stdio.h>

int log_message(Logger *logger, char *message) {
    if (logger == NULL) {
        fprintf(stderr, "Logger is NULL\n");
        return -1;
    }
    if (message == NULL) {
        fprintf(stderr, "Message is NULL\n");
        return -1;
    }

    FILE *file = fopen(logger->log_file, "a");
    if (file == NULL) {
        perror("Error opening log file");
        return -1;
    }

    fprintf(file, "%s\n", message);
    fclose(file);

    return 0;
}
