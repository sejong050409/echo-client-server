#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10

int clients[MAX_CLIENTS];
int client_count = 0;

int echo = 0;
int broadcast = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);

    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client_sock;
    }

    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_sock) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client_sock) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(char* msg, int len, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i] == sender_sock) {
            continue;
        }

        send(clients[i], msg, len, 0);
    }

    pthread_mutex_unlock(&clients_mutex);
}
void* client_handler(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buf[BUF_SIZE];

    while (1) {
        memset(buf, 0, sizeof(buf));

        int len = recv(client_sock, buf, sizeof(buf) - 1, 0);

        if (len <= 0) {
            printf("client disconnected: %d\n", client_sock);
            remove_client(client_sock);
            close(client_sock);
            break;
        }

        printf("client[%d]: %s\n", client_sock, buf);

        if (echo) {
            send(client_sock, buf, len, 0);
        }

        if (broadcast) {
            broadcast_message(buf, len, client_sock);
        }
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("syntax : echo-server <port> [-e[-b]]\n");
        return 1;
    }

    int port = atoi(argv[1]);

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            echo = 1;
        } else if (strcmp(argv[i], "-b") == 0) {
            broadcast = 1;
        }
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 10) < 0) {
        perror("listen");
        close(server_sock);
        return 1;
    }

    printf("server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        printf("client connected: %d\n", client_sock);

        add_client(client_sock);

        int* pclient = malloc(sizeof(int));
        *pclient = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
