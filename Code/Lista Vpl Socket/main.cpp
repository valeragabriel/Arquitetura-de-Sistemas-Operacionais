#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h> 

#define PORT 1046
#define NUM_CHILDREN 10 

int main() {
    int sockfd, newsockfd, pid, pid_array[NUM_CHILDREN];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Criação do socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Vincula o socket à porta
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao vincular socket");
        exit(1);
    }

    // Escuta por conexões
    if (listen(sockfd, NUM_CHILDREN) < 0) {
        perror("Erro ao escutar");
        exit(1);
    }

    printf("Aguardando conexões...\n");

    // Cria processos filhos
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Erro ao criar processo filho");
            exit(1);
        }

        if (pid == 0) {
            // Processo filho
            close(sockfd);
            
            // Conecta-se ao pai
            newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
            if (newsockfd < 0) {
                perror("Erro ao aceitar conexão");
                exit(1);
            }

            // Informa o PID ao pai
            pid = getpid();
            send(newsockfd, &pid, sizeof(pid), 0);

            // Recebe o PID sorteado do pai
            recv(newsockfd, &pid, sizeof(pid), 0);

            // Fecha a conexão com o pai
            close(newsockfd);

            if (pid_array[i] == pid) {
                printf("%d: fui sorteado\n", getpid());
            } else {
                printf("%d: não fui sorteado\n", getpid());
            }

            exit(0);
        } else {
            // Processo pai
            pid_array[i] = pid;
        }
    }

    // Processo pai gera um PID sorteado
    srand(time(NULL));
    int random_index = rand() % NUM_CHILDREN;
    int random_pid = pid_array[random_index];
    printf("PID sorteado: %d\n", random_pid);

    // Informa o PID sorteado aos filhos
    for (int i = 0; i < NUM_CHILDREN; i++) {
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (newsockfd < 0) {
            perror("Erro ao aceitar conexão");
            exit(1);
        }

        send(newsockfd, &random_pid, sizeof(random_pid), 0);
        close(newsockfd);
    }

    // Fecha o socket do pai
    close(sockfd);

    return 0;
}
