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
struct sockaddr_in peer_addr;

pthread_mutex_t stdout_lock;

char new_file[20];

// Function Prototypes
void * read_input(void *ptr);
void configure();
void generate_menu();
void add_file_tracker(char filename[FILENAME_MAX]);
void receive_packet();
void request_available_peers();
void receive_available_peers(packet *pkt);
void request_file_to_peer();
void send_file(packet *pkt, struct sockaddr_in dest_addr);
void receive_file(packet *pkt, struct sockaddr_in sender_addr);
void calc_checksum(packet *pkt);
int check_checksum(packet *pkt);

int main(int argc, char **argv)
{

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "%s\n", "[error] - erro ao criar socket.");
        abort();
    }

    configure();

    if (bind(sock, (struct sockaddr *)&self_addr, sizeof(self_addr)))
    {
        fprintf(stderr, "%s\n", "[erro] - error ao bindar.");
        abort();
    }

    // create a thread to read user input
    pthread_t input_thread;
    pthread_create(&input_thread, NULL, read_input, NULL);
    pthread_detach(input_thread);

    receive_packet();
}

struct sockaddr_in get_sockaddr_in(unsigned int ip, short port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);
    return addr;
}


void configure()
{
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    self_addr.sin_port = htons(0);

    tracker_addr.sin_family = AF_INET;
    if (inet_aton("127.0.0.1", &tracker_addr.sin_addr) == 0)
    {
        fprintf(stderr, "%s\n", "error - error parsing tracker ip.");
        abort();
    }
    tracker_addr.sin_port = htons(12360);
}

void * read_input(void *ptr)
{


    char userSelection[1];
    char filename[FILENAME_MAX];
    generate_menu();

    while(1)
    {

        bzero(userSelection, sizeof(userSelection));
        scanf(" %s", userSelection);
        switch(userSelection[0])
        {
        case 'l':
            request_available_peers();
            break;
        case 'a':
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "Qual o arquivo você deseja adicionar ao rastreador?");
            pthread_mutex_unlock(&stdout_lock);
            scanf(" %[^\n]s",filename);
            add_file_tracker(filename);
            break;
        default:
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[erro] - tipo de requisição inválida.");
            pthread_mutex_unlock(&stdout_lock);
            break;
        }


    }
}

void add_file_tracker(char filename[FILENAME_MAX])
{
    FILE *arq;

    if( access( filename, F_OK ) != -1 )
    {

        packet pkt;
        pkt.header.type = 'a';
        pkt.header.error = '\0';
        pkt.header.msg_length = strlen(filename) + 1;
        memcpy(pkt.msg, filename, pkt.header.msg_length);

        // send packet to tracker
        int status = sendto(sock, &pkt, sizeof(pkt.header) + pkt.header.msg_length, 0, (struct sockaddr *)&tracker_addr, sizeof(struct sockaddr_in));
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
    socklen_t addrlen = sizeof(struct sockaddr);
    packet pkt;
    int status;

    while(1)
    {
        bzero(&sender_addr, sizeof(sender_addr));
        bzero(&pkt, sizeof(pkt));
        status = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&sender_addr, &addrlen);
        if (status < 0)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[erro] - erro ao receber pacote, ignorando.");
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
            fprintf(stderr, "%s\n", "O rastreador adicionou o seu arquivo com sucesso!");
            pthread_mutex_unlock(&stdout_lock);
            break;
        case 's':
            send_file(&pkt, sender_addr);
            break;
        case 'r':
            receive_file(&pkt, sender_addr);
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
    bzero(filename, sizeof(filename));
    scanf(" %[^\n]s",filename);


    packet pkt;
    pkt.header.type = 'l';
    pkt.header.error = '\0';
    pkt.header.msg_length = strlen(filename);
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

void send_file(packet *recv_pkt, struct sockaddr_in dest_addr)
{

    socklen_t l = sizeof(struct sockaddr);

    FILE *arq;
    char filename[FILENAME_MAX] = "image.png";
    int cont_pac, status_arq, msg_tam;

    arq = fopen(filename, "rb");
    if(arq == NULL)
    {
        puts("Não foi possível abrir o arquivo");
    }
    else
    {
        pthread_mutex_unlock(&stdout_lock);
        printf("Enviando arquivo %s para o requisitante.\n", filename);
        pthread_mutex_unlock(&stdout_lock);

        long file_size;
        fseek(arq, 0, SEEK_END);
        file_size = ftell(arq) + 1;
        fseek(arq, 0, SEEK_SET);
        int fr;
        int numberPackets = file_size/MAX_MSG + 1;
        pthread_mutex_unlock(&stdout_lock);
        printf("numPacotes: %d\n", numberPackets);
        pthread_mutex_unlock(&stdout_lock);

        int i = 0;
        int seq = 0;



        //enviando pacotes ao requisitante
        packet sendPacket;
        packet receivedPacket;
        pthread_mutex_lock(&stdout_lock);
        for(seq = 0; seq < numberPackets; seq++)
        {
            //inicializa pacote
            sendPacket.header.ack = 0;
            sendPacket.header.seq = seq;
            sendPacket.header.type = 'r';
            sendPacket.header.error = '\0';
            sendPacket.header.msg_length = 0;

            if(seq == numberPackets - 1)
                sendPacket.header.ultimo = 1;
            else
                sendPacket.header.ultimo = 0;
            //fim da inicialização

            if ((fr = fread(sendPacket.msg, MAX_MSG, 1, arq)) < 0)
            {
                perror("erro ao pegar bytes do arquivo\n");
            }


            calc_checksum(&sendPacket); //Calcula o checksum do pacote atual


            if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, sizeof(dest_addr)) < 0)
            {
                perror("erro ao enviar pacote\n");
            }

            while(1)
            {
                // recebe o mesmo pacote que enviou pro cliente só que com ack = true preenchido pelo cliente
                if(recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, &l) < 0)
                {
                    perror("erro ao receber pacote\n");
                }

                sendPacket.header.ack = receivedPacket.header.ack;

                printf("seq: %d, checksum: %d, pacote.ack: %d\n", seq, sendPacket.header.checksum, sendPacket.header.ack);
                // se ack == false cliente nao recebeu, entao manda de novo
                if(sendPacket.header.ack == 0)
                {
                                    if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, sizeof(dest_addr)) < 0) {
                    perror("erro ao enviar pacote\n");
                }
                }
                // se ack == true sai do while e envia proximo pacote
                else
                {
                    break;
                }
            }

        }
        pthread_mutex_unlock(&stdout_lock);


        pthread_mutex_lock(&stdout_lock);
        puts("arquivo enviado com sucesso!");
        pthread_mutex_unlock(&stdout_lock);
        fclose(arq);

    }





}


void receive_file(packet *pkt, struct sockaddr_in sender_addr)
{
    // abre arquivo a ser escrito com os dados dos pacotes recebidos
    int seqR = 0;
    int checksum_ok = 0;
    FILE *f;

    if(pkt->header.seq == 0)
    {
        strcpy(new_file, "copied_");
        strcat(new_file, "image.png");
        f = fopen(new_file, "wb");
    }

    socklen_t l = sizeof(struct sockaddr);


    packet sendPacket;
    pthread_mutex_lock(&stdout_lock);
    packet receivedPacket = *pkt;
    while(receivedPacket.header.ultimo == 0)
    {

        if(seqR > 0)
        {
            bzero(&receivedPacket, sizeof(pkt));

            if (recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr *)&sender_addr, &l) < 0)
            {
                fprintf(stderr, "%s\n", "[erro] - erro ao receber pacote, ignorando.");
            }
        }

        sendPacket = *pkt;

        checksum_ok = check_checksum(pkt);

        printf("seq %d, ack %d, checksum = %d, checksum_ok = %d, ultimo = %d\n", receivedPacket.header.seq, receivedPacket.header.ack, receivedPacket.header.checksum, checksum_ok, receivedPacket.header.ultimo); //Imprime informações referentes ao pacote recebido



        if(seqR == receivedPacket.header.seq)
        {
            sendPacket.header.ack = 1;

            if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &sender_addr, sizeof(sender_addr)) < 0)
            {
                perror("erro ao enviar pacote\n");
            }

            // escreve arquivo
            if (fwrite(receivedPacket.msg, 1, MAX_MSG, f) < 0)
            {
                perror("erro ao escrever arquivo\n");
            }

            // reseta pacote para ser preenchido com informações do proximo
            bzero(receivedPacket.msg, MAX_MSG);
        }



        if(seqR == 0)
        {
            seqR++;
            continue;
        }
        else
        {
            seqR++;
        }

    }
    puts("arquivo baixado com sucesso");
    add_file_tracker("image.png");
    pthread_mutex_unlock(&stdout_lock);
    fclose(f);
    return;




    // reseta pacote para ser preenchido com informações do proximo





}

void receive_message(struct sockaddr_in *sender_addr, packet *pkt)
{
    // fetch sender information
    char *sender_ip = inet_ntoa(sender_addr->sin_addr);
    short sender_port = htons(sender_addr->sin_port);

}

void request_file_to_peer()
{

    int port;
    char ip[20];
    bzero(ip, sizeof(ip));

    printf("Digite o IP do destinatário\n");
    scanf("%[^\n]s",ip);

    printf("Digite a porta do destinatário\n");
    scanf("%d", &port);

    memset(&peer_addr, 0, sizeof (peer_addr));

    peer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    packet pkt;
    pkt.header.type = 's';
    pkt.header.error = 'e';

    int status = sendto(sock, &pkt, sizeof(pkt.header), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if (status == -1)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "error - error sending packet to tracker");
        pthread_mutex_unlock(&stdout_lock);
    }



}

void receive_available_peers(packet *recv_pkt)
{

    pthread_mutex_lock(&stdout_lock);
    if(recv_pkt->header.error == 'e')
    {
        puts("Parece que ninguém possui esse arquivo :(");

    }
    else
    {
        puts(recv_pkt->msg);
        request_file_to_peer();
    }
    pthread_mutex_unlock(&stdout_lock);

    return;
}

void calc_checksum(packet *pct)   //Calcula a soma de verificacao
{
    unsigned int i, sum = 0;
    for (i = 0; i < MAX_MSG; i++)
    {
        if (pct->msg[i] == '1') sum += 2*i;
        else sum += i;
    }

    pct->header.checksum = sum;
}

int check_checksum(packet *pct)
{
    unsigned int i, sum = 0;
    for (i = 0; i < MAX_MSG; i++)
    {
        if (pct->msg[i] == '1') sum += 2*i;
        else sum += i;
    }

    if (sum == pct->header.checksum) return 1;
    else return 0;
}
