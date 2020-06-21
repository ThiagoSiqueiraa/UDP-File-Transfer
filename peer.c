/***

██╗   ██╗███╗   ██╗██╗███████╗███████╗██╗              ██████╗ ███████╗███████╗██████╗     ██████╗
██║   ██║████╗  ██║██║██╔════╝██╔════╝██║              ██╔══██╗██╔════╝██╔════╝██╔══██╗   ██╔════╝
██║   ██║██╔██╗ ██║██║█████╗  █████╗  ██║    █████╗    ██████╔╝█████╗  █████╗  ██████╔╝   ██║
██║   ██║██║╚██╗██║██║██╔══╝  ██╔══╝  ██║    ╚════╝    ██╔═══╝ ██╔══╝  ██╔══╝  ██╔══██╗   ██║
╚██████╔╝██║ ╚████║██║██║     ███████╗██║              ██║     ███████╗███████╗██║  ██║██╗╚██████╗
 ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═╝     ╚══════╝╚═╝              ╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝ ╚═════╝
Trabalho desenvolvido por:
THIAGO DONIZETI SIQUEIRA : 2018012355
JOÃO PEDRO JOSUÉ : 2018011044
JOÃO GUILHERME TERTULIANO DE OLIVEIRA : 2019000706

Instruções de uso em: https://github.com/ThiagoSiqueiraa/UDP-File-Transfer

ou também em README.TXT

***/

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
#include <math.h>

/** VARIAVEIS GLOBAIS **/
int sock;
struct sockaddr_in tracker_addr;
struct sockaddr_in self_addr;
struct sockaddr_in peer_addr;
pthread_mutex_t stdout_lock;

/** === PRÓTOTIPOS DAS FUNÇÕES == **/
void * read_input(void *ptr);
void configure();
void generate_menu();
void add_file_tracker(char filename[FILENAME_MAX]);
void receive_packet();
void request_available_peers();
void receive_available_peers(packet *pkt);
void request_file_to_peer(char filename[FILENAME_MAX]);
void send_file(packet *pkt, struct sockaddr_in dest_addr);
void receive_file(packet *pkt, struct sockaddr_in sender_addr);
void calc_checksum(packet *pkt);
int check_checksum(packet *pkt);

int main(int argc, char **argv)
{
    struct timeval timeout;
    timeout.tv_sec = TIMER_SEC;
    timeout.tv_usec = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "%s\n", "[ERRO] - Erro ao criar socket.\n");
        abort();
    }

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)
    {
        perror("\n[ERRO] - Erro ao configurar o socket.\n");
    }

    configure();

    if (bind(sock, (struct sockaddr *)&self_addr, sizeof(self_addr)))
    {
        fprintf(stderr, "%s\n", "[ERRO] - Erro ao bindar.\n");
        abort();
    }

    //criar thread para ler as entradas do usuário
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

    tracker_addr.sin_family = AF_INET;
    if (inet_aton("127.0.0.1", &tracker_addr.sin_addr) == 0)
    {
        fprintf(stderr, "%s\n", "[ERRO] - Erro ao vincular o ip do rastreador (tracker).\n");
        abort();
    }
    tracker_addr.sin_port = htons(SRV_PORT);
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
            fprintf(stderr, "%s\n", "[?] - Qual o arquivo você deseja adicionar ao rastreador?\n");
            pthread_mutex_unlock(&stdout_lock);
            scanf(" %[^\n]s",filename);
            add_file_tracker(filename);
            break;
        default:
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[ERRO] - Tipo de requisição inválida.\n");
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
        strcpy(pkt.msg, filename);

        int status = sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&tracker_addr, sizeof(struct sockaddr_in));
        if (status < -1)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[ERRO] - Erro ao enviar pacote ao rastreador.\n");
            pthread_mutex_unlock(&stdout_lock);
        }
    }
    else
    {
        printf("[AVISO] Não foi possível encontrar o arquivo, tente novamente.\n");
        return;
    }


}

void receive_packet()
{

    struct sockaddr_in sender_addr;
    socklen_t addrlen = sizeof(struct sockaddr);
    packet pkt;
    int status;
    int num_tries = 0;

    while(num_tries < MAX_TRIES)
    {
        bzero(&sender_addr, sizeof(sender_addr));
        bzero(&pkt, sizeof(pkt));
        status = recvfrom(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&sender_addr, &addrlen);
        if (status < 0)
        {
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[ERRO] - Erro ao receber pacote, ignorando.\n");
            num_tries++;
            pthread_mutex_unlock(&stdout_lock);
            continue;
        }
        else
        {
            num_tries = 0;
        }

        switch (pkt.header.type)
        {
        case 'l':
            receive_available_peers(&pkt);
            break;
        case 'a':
            pthread_mutex_lock(&stdout_lock);
            fprintf(stderr, "%s\n", "[AVISO] O rastreador adicionou o seu arquivo com sucesso!\n");
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
            fprintf(stderr, "%s\n", "[ERRO] - Recebido pacote de tipo desconhecido.\n");
            pthread_mutex_unlock(&stdout_lock);
            break;
        }
    }
    pthread_mutex_lock(&stdout_lock);
    printf("[AVISO] Você foi desconectado por ter ficado %d segundos sem receber resposta.\n", num_tries * TIMER_SEC );
    pthread_mutex_unlock(&stdout_lock);
    close(sock);

}

void request_available_peers()
{
    pthread_mutex_lock(&stdout_lock);
    fprintf(stderr, "%s\n", "[?] Qual o arquivo você deseja fazer o download?\n");
    pthread_mutex_unlock(&stdout_lock);
    char filename[MAX_FILENAME];
    scanf(" %[^\n]s",filename);


    packet pkt;
    pkt.header.type = 'l';
    pkt.header.error = '\0';
    strcpy(pkt.msg, filename);

    int status = sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&tracker_addr, sizeof(struct sockaddr_in));
    if (status < 0)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "[ERRO] - Erro ao enviar pacote ao rastreador.\n.");
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
    int cont_pac, status_arq, msg_tam;

    arq = fopen(recv_pkt->header.filename, "rb");
    if(arq == NULL)
    {
        puts("[ERRO] Não foi possível abrir o arquivo.\n");
    }
    else
    {
        pthread_mutex_unlock(&stdout_lock);
        printf("[AVISO] Enviando arquivo %s para o requisitante.\n", recv_pkt->header.filename);
        pthread_mutex_unlock(&stdout_lock);

        long file_size;
        fseek(arq, 0, SEEK_END);
        file_size = ftell(arq);
        fseek(arq, 0, SEEK_SET);

        int fr;
        int numberPackets = file_size/MAX_MSG + 1;

        pthread_mutex_unlock(&stdout_lock);
        printf("numPacotes: %d\n", numberPackets);
        pthread_mutex_unlock(&stdout_lock);

        int i = 0;
        int seq = 0;
        int max_tries = 0;
        int status_packet = -1;
        int flag = 0;

        packet sendPacket;
        packet receivedPacket;

        pthread_mutex_lock(&stdout_lock);
        for(seq = 0; seq < numberPackets; seq++) //enviando pacotes ao requisitante
        {
            //inicializa pacote
            sendPacket.header.ack = 0;
            sendPacket.header.seq = seq;
            sendPacket.header.type = 'r';
            sendPacket.header.error = '\0';
            strcpy(sendPacket.header.filename,recv_pkt->header.filename);
            if(seq == numberPackets - 1)
                sendPacket.header.ultimo = 1;
            else
                sendPacket.header.ultimo = 0;
            max_tries = 0;
            flag = 0;
            status_packet = -1;
            //fim da inicialização

            if ((fr = fread(sendPacket.msg, 1, MAX_MSG, arq)) < 0)
            {
                perror("[ERRO] Erro ao pegar bytes do arquivo.\n");
            }

            sendPacket.header.message_size = fr;
            calc_checksum(&sendPacket); //Calcula o checksum do pacote atual

            while(status_packet == -1 && max_tries != MAX_TRIES)
            {

                if(flag == 0)
                {
                    if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, sizeof(dest_addr)) < 0)
                    {
                        perror("[ERRO] Erro ao enviar pacote.\n");
                    }
                    flag = 1;
                }
                else
                {
                    status_packet = recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, &l);
                    if(flag > 2)
                    {
                        fprintf(stderr, "%s\n", "[AVISO] - Reenviando o pacote ao cliente.\n");
                        flag = 0;
                        max_tries++;
                    }

                    else if(status_packet == -1)
                    {
                        fprintf(stderr, "%s\n", "[ERRO] - Erro ao enviar pacote, cliente pode estar ocioso, ignorando.\n");
                        flag++;
                    }
                    else if(status_packet > 0)
                    {
                        sendPacket.header.ack = receivedPacket.header.ack; //salva o ack recebido no próximo pacote a ser enviado
                        printf("seq: %d, checksum: %d, pacote.ack: %d\n", seq, sendPacket.header.checksum, sendPacket.header.ack);
                        if(sendPacket.header.ack == 0) // se ack for falso cliente nao recebeu, reenvia o pacote.
                        {
                            if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &dest_addr, sizeof(dest_addr)) < 0)
                            {
                                perror("[ERRO] Erro ao enviar pacote\n");
                            }
                        }
                        // se ack for verdadeiro, sai do laço (while) e envia proximo pacote
                        else
                            break;
                    }
                }
            }

            if(max_tries == MAX_TRIES)
            {
                break; // sai do laço
            }
        }

        if(max_tries == MAX_TRIES)
        {
            perror("[ERRO] Numero de tentátivas de reenvio maximas alcançadas, encerrando envio do pacote.\n");
            fclose(arq);
            return;
        }

        pthread_mutex_unlock(&stdout_lock);
        pthread_mutex_lock(&stdout_lock);
        puts("[SUCESSO] Arquivo enviado com sucesso!\n");
        pthread_mutex_unlock(&stdout_lock);
        fclose(arq);

    }
}

void receive_file(packet *pkt, struct sockaddr_in sender_addr)
{
    FILE *f;
    int seqReceived = 0;
    int checksum_ok = 0;
    int status_packet = -1;
    int num_tries = 0;

    socklen_t l = sizeof(struct sockaddr);
    packet sendPacket;
    packet receivedPacket = *pkt;

    if(pkt->header.seq == 0)
    {

        f = fopen(pkt->header.filename, "wb");
        if(f == NULL)
        {
            pthread_mutex_lock(&stdout_lock);
            perror("[ERRO] Não foi possível criar o arquivo.\n");
            pthread_mutex_unlock(&stdout_lock);
            return;
        }
    }

    pthread_mutex_lock(&stdout_lock);
    while(1)
    {

        if(seqReceived > 0)
        {
            bzero(&receivedPacket, sizeof(pkt));
            num_tries = 0;
            status_packet = recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr *)&sender_addr, &l);
            while(status_packet == -1 && num_tries != MAX_TRIES)
            {
                status_packet = recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr *)&sender_addr, &l);
                if(status_packet == -1)
                {
                    fprintf(stderr, "%s\n", "[ERRO] - Erro ao receber pacote, nenhum dado recebido ou tempo limite esgotado, ignorando.\n");
                    num_tries++;
                }
                else
                {
                    num_tries = 0;
                }
            }

            if(num_tries == MAX_TRIES)
            {
                fprintf(stderr, "%s\n", "[ERRO] - Numero de tentativas permitidas atingidas, encerrado recebimento do arquivo.\n");
                return; // retorna ao receive_packet()
            }
        }

        sendPacket = *pkt;
        checksum_ok = check_checksum(pkt); // checa se o checksum dos pacotes batem.

        printf("seq %d, ack %d, checksum = %d, checksum_ok = %d, ultimo = %d\n", receivedPacket.header.seq, receivedPacket.header.ack, receivedPacket.header.checksum, checksum_ok, receivedPacket.header.ultimo); //Imprime informações referentes ao pacote recebido

        while(checksum_ok != 1) // caso o checksum não bata, espera pelo reenvio do mesmo pacote
        {
            num_tries = 0;
            status_packet = recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr *)&sender_addr, &l);
            while(status_packet == -1 && num_tries != MAX_TRIES)
            {
                status_packet = recvfrom(sock, &receivedPacket, sizeof(packet), 0, (struct sockaddr *)&sender_addr, &l);
                if(status_packet == -1)
                {
                    fprintf(stderr, "%s\n", "[ERRO] - Erro ao receber pacote, nenhum dado recebido ou tempo limite esgotado, ignorando.\n");
                    num_tries++;
                }
                else
                {
                    num_tries = 0;
                }
            }

            if(num_tries == MAX_TRIES)
            {
                fprintf(stderr, "%s\n", "[ERRO] - Número de tentativas permitidas atingidas, encerrado recebimento do arquivo.\n");
                pthread_mutex_unlock(&stdout_lock);
                return; // retorna ao receive_packet()
            }
        }


        if(seqReceived == receivedPacket.header.seq)
        {
            seqReceived++;
            sendPacket.header.ack = 1;

            if (sendto(sock, &sendPacket, sizeof(packet), 0, (struct sockaddr * ) &sender_addr, sizeof(sender_addr)) < 0)
            {
                perror("[ERRO] - Erro ao enviar pacote\n");
            }

            // escreve no arquivo
            if (fwrite(receivedPacket.msg, 1, receivedPacket.header.message_size, f) < 0)
            {
                perror("[ERRO] - Erro em escrever no arquivo\n");
            }

            // reseta msg do pacote.
            bzero(receivedPacket.msg, MAX_MSG);
        }

        if(receivedPacket.header.ultimo == 1) //caso o pacote seja o último sai do laço de repetição
            break;

    }
    puts("[SUCESSO] Arquivo foi baixado com sucesso");
    add_file_tracker(pkt->header.filename); //informa ao rastreador que agora o peer t ambém possui o pacote
    pthread_mutex_unlock(&stdout_lock);
    fclose(f);
    return;
}

void request_file_to_peer(char file[FILENAME_MAX])
{

    int port;
    char ip[INET_ADDRSTRLEN];
    bzero(ip, sizeof(ip));

    printf("[?] Digite o IP do destinatário\n");
    scanf("%[^\n]s",ip);

    printf("[?] Digite a porta do destinatário\n");
    scanf("%d", &port);

    memset(&peer_addr, 0, sizeof (peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    packet pkt;
    pkt.header.type = 's';
    pkt.header.error = '\0';
    strcpy(pkt.header.filename, file);

    int status = sendto(sock, &pkt, sizeof(pkt.header), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
    if (status == -1)
    {
        pthread_mutex_lock(&stdout_lock);
        fprintf(stderr, "%s\n", "[ERRO] - Erro ao enviar pacote ao rastreador.\n");
        pthread_mutex_unlock(&stdout_lock);
    }



}

/**
void receive_available_peers(packet *recv_pkt)
Função que irá receber as listas de usuários (peer) que possuem o arquivo requisitado,
caso o pacote contenha error = 'e' ninguém tem o pacote, caso contrário,
imprime os peers disponíveis
**/
void receive_available_peers(packet *recv_pkt)
{

    pthread_mutex_lock(&stdout_lock);
    if(recv_pkt->header.error == 'e')
    {
        puts("Parece que ninguém possui esse arquivo :(\n");

    }
    else
    {
        puts(recv_pkt->msg);
        request_file_to_peer(recv_pkt->header.filename);
    }
    pthread_mutex_unlock(&stdout_lock);

    return;
}

/**
void calc_checksum(packet *pct)
calcula o checksum da mensagem,
e retorna para o pacaote (passado como pointer)**/
void calc_checksum(packet *pct)
{
    unsigned int i, sum = 0;
    for (i = 0; i < MAX_MSG; i++)
    {
        if (pct->msg[i] == '1') sum += 2*i;
        else sum += i;
    }

    pct->header.checksum = sum;
}

/**
int check_checksum(packet *pct);
Recebe um pacote, calcula o checksum da mensagem,
caso seja igual retorna 1
caso contrário retorna 0 **/
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
