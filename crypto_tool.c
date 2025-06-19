#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cyphers.h"

void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <encrypt|decrypt> <aes|chacha> <input_file> <output_file>\n", progname);
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        print_usage(argv[0]);
    }

    int action = -1;
    if (strcmp(argv[1], "encrypt") == 0)
        action = 0;
    else if (strcmp(argv[1], "decrypt") == 0)
        action = 1;
    else
        print_usage(argv[0]);

    int alg = -1;
    if (strcmp(argv[2], "aes") == 0)
        alg = 0;
    else if (strcmp(argv[2], "chacha") == 0)
        alg = 1;
    else
        print_usage(argv[0]);

    const char *input_file = argv[3];
    const char *output_file = argv[4];
    const char password[8] = "aaaaaaa\0";

    run_symmetric(alg, action, input_file, output_file, password);

    return 0;
}