#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include "message.h"

int sock;
struct sockaddr_in tracker_addr;
struct sockaddr_in self_addr;
struct sockaddr_in peer_list[100];
unsigned int room_num = 0;
int peer_num = 0;

pthread_mutex_t stdout_lock;
pthread_mutex_t peer_list_lock;

// Function Prototypes
void * read_input(void *ptr);
void configure();
void generate_menu();
void add_file_tracker();
void receive_packet();
void request_available_peers();
void receive_available_peers(packet *pkt);
void select_peer_to_connect();
void send_file();
void receive_file(char *file);


int main(int argc, char **argv)
{

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "%s\n", "error - error creating socket.");
        abort();
    }



    configure();


    if (bind(sock, (struct sockaddr *)&self_addr, sizeof(self_addr)))
    {
        fprintf(stderr, "%s\n", "error - error binding.");
        abort();
    }

    // setup_test_peers();

    // create a thread to read user input
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, read_input, NULL);
    pthread_detach(input_thread);

    receive_packet();
}

void configure()
{
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    self_addr.sin_port = htons(0);

    short tracker_port = 12360;
    char tracker_ip[20] = "127.0.0.1";

    tracker_addr.sin_family = AF_INET;
    if (inet_aton(tracker_ip, &tracker_addr.sin_addr) == 0)
    {
        fprintf(stderr, "%s\n", "error - error parsing tracker ip.");
        abort();
    }
    tracker_addr.sin_port = htons(tracker_port);
}

void * read_input(void *ptr)
{

        generate_menu();

        char userSelection;

    while(1)
    {


        scanf(" %c", &userSelection);

        switch(userSelection)
        {
        case 'l':
            request_available_peers();
            break;
        case 'a':
            add_file_tracker();
            break;
        default:
				pthread_mutex_lock(&stdout_lock);
				fprintf(stderr, "%s\n", "[erro] - tipo de requisição inválida.");
				pthread_mutex_unlock(&stdout_lock);
				break;
        }

    }
    return NULL;
}

void add_file_tracker()
{
    FILE *arq;
    char filename[1000];

    pthread_mutex_lock(&stdout_lock);
    fprintf(stderr, "%s\n", "Qual o arquivo você deseja adicionar ao rastreador?");
    pthread_mutex_unlock(&stdout_lock);

    scanf(" %[^\n]s",filename);
    if( access( filename, F_OK ) != -1 )
    {

        packet pkt;
        pkt.header.type = 'a';
        pkt.header.error = '\0';
        pkt.header.filename_length = strlen(filename) + 1;
        memcpy(pkt.header.filename, filename, pkt.header.filename_length);

        // send packet to tracker
        int status = sendto(sock, &pkt, sizeof(pkt.header), 0, (struct sockaddr *)&tracker_addr, sizeof(struct sockaddr_in));
        if (status < -1)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "error - error sending packet to tracker");
            pthread_mutex_unlock(&stdout_lock);
        }
    }
    else
    {
        printf("Não foi encontrar o arquivo, tente novamente.\n");
        return;
    }


}

void receive_packet()
{

    struct sockaddr_in sender_addr;
    socklen_t addrlen = 10;
    packet pkt;
    int status;

    while (1)
    {
        status = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&sender_addr, &addrlen);
        if (status == -1)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "error - error receiving a packet, ignoring.");
            pthread_mutex_unlock(&stdout_lock);
            continue;
        }

        switch (pkt.header.type)
        {
        case 'l':
            receive_available_peers(&pkt);
            break;
        case 'a':
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "Você adicionou o arquivo com sucesso!");
            pthread_mutex_unlock(&stdout_lock);
            break;
        case 'g':
            send_file()
            break;
        default:
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "error - received packet type unknown.");
            pthread_mutex_unlock(&stdout_lock);
            break;
        }
    }
}

void request_available_peers()
{
    pthread_mutex_lock(&stdout_lock);
    fprintf(stderr, "%s\n", "Qual o arquivo você deseja fazer o download?");
    pthread_mutex_unlock(&stdout_lock);
    char filename[1000];
    scanf(" %[^\n]s",filename);


    packet pkt;
    pkt.header.type = 'l';
    pkt.header.error = '\0';
    pkt.header.msg_length = strlen(filename) + 1;
    memcpy(pkt.msg, filename, pkt.header.msg_length);


    // send packet to tracker
    int status = sendto(sock, &pkt, sizeof(pkt.header) + pkt.header.msg_length, 0, (struct sockaddr *)&tracker_addr, sizeof(struct sockaddr_in));
    if (status < 0)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "error - error sending packet to tracker");
        pthread_mutex_unlock(&stdout_lock);
    }
}

void generate_menu()
{
    printf("Olá querido usuário, por favor selecione uma das opções.\n");
    printf("[l]\t-\t Consultar listas de clientes de um arquivo e baixar\n");
    printf("[a]\t-\t Informar ao rastreador que possuo determinado arquivo\n");
}

void send_file(char *msg){

	if (msg[0] == '\0') {
		pthread_mutex_lock(&stdout_lock);
		fprintf(stderr, "%s\n", "error - no message content.");
		pthread_mutex_unlock(&stdout_lock);
	}

		// format packet
	packet pkt;
	pkt.header.type = 'm';
	pkt.header.error = '\0';
	pkt.header.msg_length = strlen(msg) + 1;
	memcpy(pkt.msg, msg, pkt.header.msg_length);

	// send packet to every peer
	int i;
	int status;
    status = sendto(sock, &pkt, sizeof(pkt.header) + pkt.header.msg_length, 0, (struct sockaddr *)&(peer_list[i]), sizeof(struct sockaddr_in));
    if (status == -1) {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s %d\n", "error - error sending packet to peer", i);
        pthread_mutex_unlock(&stdout_lock);
    }

}

void receive_message(struct sockaddr_in *sender_addr, packet *pkt) {
	// fetch sender information
	char *sender_ip = inet_ntoa(sender_addr->sin_addr);
	short sender_port = htons(sender_addr->sin_port);

}

void select_peer_to_connect(){

    char ip[20];
    int port;

    printf("Entre com o IP que deseja se conectar\n");
    scanf(" %[^\n]s",ip);
    printf("Entre com a porta que deseja se conectar\n");
    scanf("%d", &port);


    struct sockaddr_in peer_addr;
    memset (&peer_addr, 0, sizeof (peer_addr));
    peer_addr.sin_addr.s_addr = htonl(INADDR_ANY );
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    int peer_addr_fd = socket(AF_INET, SOCK_DGRAM, 0);   //SOCK stream used for TCP Connections
    if (peer_addr_fd < 0)
    {
        printf("[error] - Não foi possível criar o soquete, há algo errado com o seu sistema :(");
        return;
    }




    packet pkt;
    pkt.header.type = 'g';
    pkt.header.error = 'e';


    int status = sendto(sock, &pkt, sizeof(pkt.header), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "error - error sending packet to peer");
        pthread_mutex_unlock(&stdout_lock);
    }

    return;


}

void receive_available_peers(packet *recv_pkt){

	pthread_mutex_lock(&stdout_lock);
    if(recv_pkt->header.error == 'e'){
        puts("VAZIO");
            select_peer_to_connect();
    }else{
        puts(recv_pkt->msg);
        select_peer_to_connect();
    }
	pthread_mutex_unlock(&stdout_lock);

	return;
}

void send_file(packet *pkt){

}
