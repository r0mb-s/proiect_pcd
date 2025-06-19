#include <string.h>
#include <uuid/uuid.h>

struct Header {
    int up_down;
    int first_middle_last;
    int algorithm;
    int key_len;
    char key[255];
    int enc_dec;
    uuid_t user_uuid;
    uuid_t job_uuid;
} typedef Header;

void init_header(Header *header) {
    header->up_down = -1;
    header->first_middle_last = -1;
    header->algorithm = -1;
    header->key_len = -1;
    header->key[0] = '\0';
    header->enc_dec = -1;
    memset(header->user_uuid, 0, sizeof(uuid_t));
    memset(header->job_uuid, 0, sizeof(uuid_t));
}

void make_packet(Header *header, char *buffer, int buffer_len){
    memset(buffer, 0, buffer_len);
    memcpy(buffer, header, sizeof(Header));
}