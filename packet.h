#ifndef PACKET_H
#define PACKET_H

#include <uuid/uuid.h>

struct Header {
    int up_down;
    int first_middle_last;
    int algorithm;
    int key_len;
    char key[255];
    int enc_dec;
    int message_len;
    int client_socket;
    uuid_t user_uuid;
    uuid_t job_uuid;
} typedef Header;

void init_header(Header *header);
void make_packet(Header *header, char *buffer, int buffer_len);
void modify_header(Header *header, int up_down, int first_middle_last, int message_len, int algorithm, int key_len, char *key, int enc_dec, uuid_t user_uuid, uuid_t job_uuid);
void modify_header_no_uuid(Header *header, int up_down, int first_middle_last, int message_len, int algorithm, int key_len, char *key, int enc_dec);
void header_from_buffer(Header *header, char *buffer, int buffer_len);
void print_header(Header *header);

#endif