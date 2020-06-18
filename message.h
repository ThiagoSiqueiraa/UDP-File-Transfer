#define C_SRV_PORT 	12360
#define MAX_MSG 1000
#define MAX_FILENAME 1000
#define MAX_FILES 999
#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

typedef struct message_header_t
{

    char type;
    char error;
    char filename[MAX_FILENAME];
    unsigned int filename_length;
    unsigned int msg_length;
} message_header;

typedef struct packet_t
{
    struct message_header_t header;
    char msg[MAX_MSG];
} packet;

typedef struct peer_t{
    char *ip;
    int port;
    char filename[1000];
} peer;
#endif // MESSAGE_H_INCLUDED
