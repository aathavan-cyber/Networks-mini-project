#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>

#define BUF 1024

void get_timestamp(char *buf, int size) {
    time_t now = time(NULL);
    strftime(buf, size, "%F %T", localtime(&now));
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    struct addrinfo hints, *res, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(argv[1], "2002", &hints, &res);

    for (p = res; p; p = p->ai_next) {
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if (sockfd < 0) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(sockfd);
    }

    freeaddrinfo(res);

    if (!p) {
        printf("connect failed\n");
        return 1;
    }

    printf("Connected to server\n");

    char msg[BUF], sendbuf[BUF], recvbuf[BUF];

    while (1) {
        printf("Enter message: ");
        fgets(msg, BUF, stdin);

        if (!strcmp(msg, "exit\n")) break;

        msg[strcspn(msg,"\n")] = 0;

        char ts[64];
        get_timestamp(ts, sizeof ts);

        snprintf(sendbuf, BUF, "%s|%s", ts, msg);
        send(sockfd, sendbuf, strlen(sendbuf), 0);

        int n = recv(sockfd, recvbuf, BUF-1, 0);
        if (n <= 0) break;

        recvbuf[n] = 0;
        printf("Server: %s\n", recvbuf);
    }

    close(sockfd);
}
