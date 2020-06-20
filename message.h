#define C_SRV_PORT 	12360
#define MAX_MSG 1024
#define MAX_FILENAME 200
#define MAX_FILES 999
#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

typedef struct peer_t{
    char *ip;
    int port;
    char filename[MAX_FILENAME];
} peer;

typedef struct message_header_t
{
    int ack;
    int seq;
    int ultimo;
    int checksum;
    char type;
    char error;
    unsigned int msg_length;
} message_header;

typedef struct packet_t
{
    struct message_header_t header;
    char msg[MAX_MSG];
} packet;


#endif // MESSAGE_H_INCLUDED
