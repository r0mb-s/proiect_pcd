#ifndef LOG_H
#define LOG_H

typedef struct Logger {
    char *log_file;
} Logger;

int log_message(Logger *logger, char *message);

#endif
