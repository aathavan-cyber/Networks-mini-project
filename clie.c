// echo_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void get_timestamp(char *buf, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(2002);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to server\n");

    char message[BUFFER_SIZE];
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    while (1) {
        printf("Enter message (exit to quit): ");
        fgets(message, BUFFER_SIZE, stdin);

        if (strcmp(message, "exit\n") == 0)
            break;

        message[strcspn(message, "\n")] = 0;

        char timestamp[64];
        get_timestamp(timestamp, sizeof(timestamp));

        snprintf(sendbuf, sizeof(sendbuf), "%s|%s", timestamp, message);

        send(sockfd, sendbuf, strlen(sendbuf), 0);

        int bytes = recv(sockfd, recvbuf, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        recvbuf[bytes] = '\0';
        printf("Server reply: %s\n", recvbuf);
    }

    close(sockfd);
    return 0;
}
