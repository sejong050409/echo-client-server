#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024

void* recv_thread(void* arg) {
    int sock = *(int*)arg;
    char buf[BUF_SIZE];

    while (1) {
        memset(buf, 0, sizeof(buf));

        int len = recv(sock, buf, sizeof(buf) - 1, 0);

        if (len <= 0) {
            printf("server disconnected\n");
            close(sock);
            exit(0);
        }

        printf("server: %s\n", buf);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("syntax : echo-client <ip> <port>\n");
        return 1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("connected to server\n");

    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, &sock);
    pthread_detach(tid);

    char msg[BUF_SIZE];

    while (1) {
        memset(msg, 0, sizeof(msg));

        if (fgets(msg, sizeof(msg), stdin) == NULL) {
            break;
        }

        msg[strcspn(msg, "\n")] = '\0';

        if (strlen(msg) == 0) {
            continue;
        }

        send(sock, msg, strlen(msg), 0);
    }

    close(sock);
    return 0;
}
