#include "packet.h"
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

void init_header(Header *header) {
    header->up_down = -1;
    header->first_middle_last = -1;
    header->algorithm = -1;
    header->key_len = -1;
    header->key[0] = '\0';
    header->enc_dec = -1;
    header->message_len = -1;
    memset(header->user_uuid, 0, sizeof(uuid_t));
    memset(header->job_uuid, 0, sizeof(uuid_t));
}

void make_packet(Header *header, char *buffer, int buffer_len) {
    memcpy(buffer, header, sizeof(Header));
}

void header_from_buffer(Header *header, char *buffer, int buffer_len){
    memcpy(header, buffer, sizeof(Header));
}

void modify_header(Header *header, int up_down, int first_middle_last, int message_len, int algorithm, int key_len, char *key, int enc_dec, uuid_t user_uuid, uuid_t job_uuid) {
    init_header(header);
    
    header->up_down = up_down;
    header->first_middle_last = first_middle_last;
    header->algorithm = algorithm;
    header->key_len = key_len;
    header->message_len = message_len;
    strcpy(header->key, key);
    header->enc_dec = enc_dec;
    memcpy(header->user_uuid, user_uuid, sizeof(uuid_t));
    memcpy(header->job_uuid, job_uuid, sizeof(uuid_t));
}

void modify_header_no_uuid(Header *header, int up_down, int first_middle_last, int message_len, int algorithm, int key_len, char *key, int enc_dec) {
    init_header(header);
    
    header->up_down = up_down;
    header->first_middle_last = first_middle_last;
    header->algorithm = algorithm;
    header->key_len = key_len;
    header->message_len = message_len;
    strcpy(header->key, key);
    header->enc_dec = enc_dec;
}

void print_header(Header *header){
    char user_uuid[37], job_uuid[37];
    uuid_unparse(header->user_uuid, user_uuid);
    uuid_unparse(header->job_uuid, job_uuid);
    printf("Upload/Download: %d\nFirst/Middle/Last: %d\nMessage length: %d\nAlgorithm: %d\nKey length: %d\nKey: %s\nEnc/Dec: %d\nUser UUID: %s\nJob UUID: %s\n", header->up_down, header->first_middle_last, header->message_len, header->algorithm, header->key_len, header->key, header->enc_dec, user_uuid, job_uuid);
}