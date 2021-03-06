/***

██╗   ██╗███╗   ██╗██╗███████╗███████╗██╗
██║   ██║████╗  ██║██║██╔════╝██╔════╝██║
██║   ██║██╔██╗ ██║██║█████╗  █████╗  ██║
██║   ██║██║╚██╗██║██║██╔══╝  ██╔══╝  ██║
╚██████╔╝██║ ╚████║██║██║     ███████╗██║
 ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═╝     ╚══════╝╚═╝
Trabalho desenvolvido por:
THIAGO DONIZETI SIQUEIRA : 2018012355
JOÃO PEDRO JOSUÉ : 2018011044
JOÃO GUILHERME TERTULIANO DE OLIVEIRA : 2019000706

Instruções de uso em: https://github.com/ThiagoSiqueiraa/UDP-File-Transfer

ou também em README.TXT

***/


#define SRV_PORT 	13360
#define MAX_MSG 1024
#define MAX_FILENAME 50
#define MAX_FILES 999
#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED
#define TIMER_SEC 60
#define MAX_TRIES 3
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
    char filename[MAX_FILENAME];
    int message_size;
} message_header;

typedef struct packet_t
{
    struct message_header_t header;
    char msg[MAX_MSG];
} packet;


#endif // MESSAGE_H_INCLUDED
