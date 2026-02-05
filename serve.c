// echo_server_threaded.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 2002
#define BUFFER_SIZE 1024

int msg_count = 0;
pthread_mutex_t lock;

void *handle_client(void *arg) {
    int clientfd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes = recv(clientfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';

        // split timestamp and message
        char *timestamp = strtok(buffer, "|");
        char *message = strtok(NULL, "|");

        // protect shared counter
        pthread_mutex_lock(&lock);
        msg_count++;
        int current_count = msg_count;
        pthread_mutex_unlock(&lock);

        char reply[BUFFER_SIZE];
        snprintf(reply, sizeof(reply),
                 "Echo: %s | ClientTime: %s | Count: %d",
                 message ? message : "",
                 timestamp ? timestamp : "",
                 current_count);

        send(clientfd, reply, strlen(reply), 0);
    }

    close(clientfd);
    printf("Client disconnected\n");
    return NULL;
}

int main() {
    int servsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (servsockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(servsockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(servsockfd, 10) < 0) {
        perror("listen");
        return 1;
    }

    pthread_mutex_init(&lock, NULL);

    printf("Threaded Echo Server listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);

        int *clientfd = malloc(sizeof(int));
        *clientfd = accept(servsockfd, (struct sockaddr*)&clientaddr, &len);

        if (*clientfd < 0) {
            perror("accept");
            free(clientfd);
            continue;
        }

        printf("Client connected\n");

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, clientfd);
        pthread_detach(tid);  // auto cleanup
    }

    pthread_mutex_destroy(&lock);
    close(servsockfd);
    return 0;
}
