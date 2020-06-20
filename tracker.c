#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<string.h>    //strlen, puts
#include<stdio.h>
#include<stdlib.h> //exit
#include<netinet/in.h>
#include<stdlib.h>
#include<pthread.h>
#include<netdb.h>
#include<string.h>
#include "message.h"

#define NUM_OF_USERS 10 //total number of users that can be register
#define MAX_USERS 50


void *handlePacket(void *args);
void user_add(struct sockaddr_in client, packet pkt);
void list_peer(struct sockaddr_in client, packet recv_pkt);

static unsigned int users_count = 0;
static peer listOfPeerFiles[MAX_FILES]; //global array of connected users to the system. notice the {0}initializer it is very important!
static int port_cnt = 0;


int socket_fd;
pthread_mutex_t stdout_lock;

int main(int argc, char*argv[]) //***check all if for exit the program or not.
{

    memset(listOfPeerFiles,0,MAX_FILES*sizeof(peer));

    /*Define vars for communication*/
    int client_fd;
    struct sockaddr_in serv_sck,client_sock;
    int n;
    short unsigned int port;
    struct hostent *hostp;
    char *hostaddrp;

    /*******************************/
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);   //SOCK stream used for TCP Connections
    if (socket_fd == -1)
    {
        printf("[error] - Não foi possível criar o soquete, há algo errado com o seu sistema :(");
        return 1;
    }

    puts("Socket criado");

    //clear! and Prepare the sockaddr_in structure
    // bzero((char *) &serv_sck, sizeof(serv_sck));
    memset(&serv_sck, 0, sizeof(serv_sck));

    serv_sck.sin_family = AF_INET; //IPv4 Structure
    serv_sck.sin_addr.s_addr = htonl(INADDR_ANY); //opened to any IP
    serv_sck.sin_port = htons( C_SRV_PORT ); //port num defined in chat.h

    //Bind socket
    if( bind(socket_fd,(struct sockaddr *)&serv_sck, sizeof(serv_sck)) < 0)
    {
        //print the error message
        perror("[error] - Falha ao bindar socket :(");
        return 1;
    }
    puts("Bind concluída");

    puts("Tudo certo, aguardando conexões...");
    int clientlen = sizeof(client_sock);
    packet recv_pkt;
    while(1)
    {
        memset(&client_sock, 0, sizeof(client_sock));
        memset(&recv_pkt, 0, sizeof(recv_pkt));
        n = recvfrom(socket_fd, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&client_sock, &clientlen);
        if(n < 0)
        {
            perror("[error] Ao tentar acertar o pacote :(");
        }
        else
        {
            unsigned int ip = client_sock.sin_addr.s_addr;
            int port = ntohs(client_sock.sin_port);
            hostaddrp = inet_ntoa(client_sock.sin_addr);
            if(hostaddrp == NULL)
                return 1;
            puts("Conexão com cliente aceita!");
            switch(recv_pkt.header.type)
            {
            case 'l':
                list_peer(client_sock, recv_pkt);
                break;
            case 'a':
                user_add(client_sock, recv_pkt);
                break;
            default:
                fprintf(stderr, "%s", "erro ao receber pacote, ignorando");
                break;
            }
        }
    }

    close(n);

    return 0;
}

/* Add user to userList */


struct sockaddr_in get_sockaddr_in(unsigned int ip, short port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);
    return addr;
}

void user_add(struct sockaddr_in client, packet p_pkt)
{

    packet pkt;
    pkt.header.type = 'a';
    pkt.header.error = '\0';


    unsigned int ip = client.sin_addr.s_addr;
    int port = ntohs(client.sin_port);

    int i;
    peer p;
    int flag = 0;

    for(int i = 0; i < MAX_FILES; i++)
    {


        if(listOfPeerFiles[i].ip == 0)
        {

            strcpy(p.filename, p_pkt.msg);
            p.port = port;
            p.ip = inet_ntoa(client.sin_addr);
            listOfPeerFiles[i] = p;
            printf("[%d]\t-\t Arquivo : %s\n\n",i,listOfPeerFiles[i].filename);
            printf("[%d]\t-\t IP : %s\n\n",i,listOfPeerFiles[i].ip);
            printf("[%d]\t-\t Porta : %d\n\n",i,listOfPeerFiles[i].port);
            break;
        }

    }

    struct sockaddr_in peer_addr = get_sockaddr_in(ip, port);
    int status = sendto(socket_fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if (status < 0)
    {
        perror("[error] - Erro ao enviar pacote para o peer");
    }
}

void list_peer(struct sockaddr_in client_sock, packet recv_pkt)
{
    unsigned int ip = client_sock.sin_addr.s_addr;
    int port = ntohs(client_sock.sin_port);
    char *hostaddrp = inet_ntoa(client_sock.sin_addr);

    printf("Enviando ao cliente (%s:%d) a lista de usuários que possuem o arquivo: (%s)\n", hostaddrp, port, recv_pkt.msg);

    packet pkt;
    pkt.header.type = 'l';
    pkt.header.error = '\0';

    struct sockaddr_in peer_addr = get_sockaddr_in(ip, port);

    int flag = 0;

    char *list_entry_format = (char *)"Peer[%d]: %s:%d\n";
    int list_entry_size = sizeof (char*) + sizeof(int) + (sizeof(int)) + strlen(list_entry_format);
    int list_size = MAX_FILES*list_entry_size;
    char *list_entry = (char *)malloc(list_entry_size);
    char *list = (char *)malloc(list_size);
    unsigned int i;
    char *list_i = list;

    for(i = 0; i < MAX_FILES; i++)
    {
        if(strcmp(listOfPeerFiles[i].filename, recv_pkt.msg) == 0 && listOfPeerFiles[i].port != port){
                flag = 1;
                sprintf(list_entry, list_entry_format, i, listOfPeerFiles[i].ip, listOfPeerFiles[i].port);
                strcat(list_i, list_entry);
                list_i += strlen(list_i);

        }
    }


    if(flag == 0)
    {
        pkt.header.error = 'e';
    }
    else
    {
        strcpy(pkt.msg, list);
    }
    int status = sendto(socket_fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));

    if (status == -1)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "error - error sending packet to peer");
        pthread_mutex_unlock(&stdout_lock);
    }

}


