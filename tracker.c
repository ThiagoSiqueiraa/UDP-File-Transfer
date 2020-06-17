/*
 * chats.c
 *
 *  Created on: Jun 6, 2017
 *      Author: Eran Peled
 *
 *      This File is the implement of the "Server"
 *      During this exercise the server is a kind of "DB" he is responsible keeping records of users in the system
 *      and provide a list of current connected users to the system to any given client.
 */
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
#include "chat.h"

#define NUM_OF_USERS 10 //total number of users that can be register
#define MAX_USERS 50


void *handlePacket(void *args);
void user_add(unsigned int ip, short port, packet pkt);
void user_delete(msg_down_t* msg);
void list_peer(unsigned int ip, short port, char filename[]);
void selectPeerToConnect();

static unsigned int users_count = 0;
static msg_peer_t *listOfUsers[MAX_USERS] = {0}; //global array of connected users to the system. notice the {0}initializer it is very important!
static int port_cnt = 0;


int socket_fd;
pthread_mutex_t stdout_lock;

int main(int argc, char*argv[]) //***check all if for exit the program or not.
{

    /*Define vars for communication*/
    int client_fd;
    struct sockaddr_in serv_sck,client_sock;
    int n;
    short unsigned int port;
    struct hostent *hostp;
    char *hostaddrp;

    /*******************************/
    fprintf(stderr, "Starting server on port: %d\n", C_SRV_PORT);
    //Create socket for IPv4
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);   //SOCK stream used for TCP Connections
    if (socket_fd < 0)
    {
        printf("Could not create socket, there is something wrong with your system");
    }
    puts("Created socket");

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
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    puts("Waiting For Clients...");
    int clientlen = sizeof(client_sock);
    packet recv_pkt;
    while(1)
    {
        n = recvfrom(socket_fd, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *)&client_sock, &clientlen);
        if(n == -1)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s", "erro ao receber pacote, ignorando");
            pthread_mutex_unlock(&stdout_lock);
        }
        else
        {
            unsigned int ip = client_sock.sin_addr.s_addr;
            int port = ntohs(client_sock.sin_port);
            hostaddrp = inet_ntoa(client_sock.sin_addr);
            if(hostaddrp == NULL)
                return 1;
            switch(recv_pkt.header.type)
            {
            case 'l':
                pthread_mutex_lock(&stdout_lock);
                printf("Enviando ao cliente (%s:%d) a lista de usuários que possuem o arquivo: (%s)\n", hostaddrp, port, recv_pkt.payload);
                pthread_mutex_unlock(&stdout_lock);
                list_peer(ip, port, recv_pkt.payload);
                break;
            case 'a':

                user_add(ip, port, recv_pkt);
                break;
            default:
                pthread_mutex_lock(&stdout_lock);
                fprintf(stderr, "%s", "erro ao receber pacote, ignorando");
                pthread_mutex_unlock(&stdout_lock);
                break;
            }
        }
    }

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

void user_add(unsigned int ip, short port, packet pkt){


}

void list_peer(unsigned int ip, short port, char filename[1000])
{
    packet pkt;
    pkt.header.type = 'l';
    pkt.header.error = '\0';

    pthread_mutex_lock(&stdout_lock);
    fprintf(stderr, "Enviando ao cliente (%s:%d) a lista de usuários\n", filename, 333);
    pthread_mutex_unlock(&stdout_lock);



    struct sockaddr_in peer_addr = get_sockaddr_in(ip, port);
    int status = sendto(socket_fd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if (status == -1)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "error - error sending packet to peer");
        pthread_mutex_unlock(&stdout_lock);
    }

}



