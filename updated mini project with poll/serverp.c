#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define PORT "2002"
#define BUF 1024

int msg_count = 0;

void add_fd(struct pollfd **pfds, int fd, int *count, int *size) {
    if (*count == *size) {
        *size *= 2;
        *pfds = realloc(*pfds, (*size) * sizeof(struct pollfd));
    }
    (*pfds)[*count].fd = fd;
    (*pfds)[*count].events = POLLIN;
    (*pfds)[*count].revents = 0;
    (*count)++;
}

/* remove fd */
void del_fd(struct pollfd pfds[], int i, int *count) {
    close(pfds[i].fd);
    pfds[i] = pfds[*count - 1];
    (*count)--;
}

int make_listener() {
    struct addrinfo hints, *res, *p;
    int sockfd, yes = 1, rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;          // force IPv6 socket
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = res; p; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        /* ⭐ allow IPv4-mapped connections */
        int no = 0;
        setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(sockfd);
    }

    freeaddrinfo(res);

    if (!p) return -1;

    if (listen(sockfd, 20) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main() {
    int listener = make_listener();
    if (listener < 0) {
        fprintf(stderr, "Failed to create listener\n");
        exit(1);
    }

    int fd_size = 10, fd_count = 0;
    struct pollfd *pfds = malloc(fd_size * sizeof *pfds);

    add_fd(&pfds, listener, &fd_count, &fd_size);

    printf("Dual-stack poll echo server on port %s\n", PORT);

    while (1) {
        int ready = poll(pfds, fd_count, -1);
        if (ready < 0) {
            perror("poll");
            break;
        }

        for (int i = 0; i < fd_count; i++) {

            if (!(pfds[i].revents & (POLLIN | POLLHUP)))
                continue;

            /* new connection */
            if (pfds[i].fd == listener) {
                int clientfd = accept(listener, NULL, NULL);
                if (clientfd < 0) {
                    perror("accept");
                    continue;
                }

                add_fd(&pfds, clientfd, &fd_count, &fd_size);
                printf("Client connected fd=%d\n", clientfd);
            }

            /* client data */
            else {
                char buf[BUF];
                int n = recv(pfds[i].fd, buf, BUF-1, 0);

                if (n <= 0) {
                    printf("Client disconnected fd=%d\n", pfds[i].fd);
                    del_fd(pfds, i, &fd_count);
                    i--;
                    continue;
                }

                buf[n] = 0;

                char *timestamp = strtok(buf, "|");
                char *message = strtok(NULL, "|");

                msg_count++;

                char reply[BUF];
                snprintf(reply, sizeof reply,
                    "Echo:%s | ClientTime:%s | Count:%d",
                    message ? message : "",
                    timestamp ? timestamp : "",
                    msg_count);

                send(pfds[i].fd, reply, strlen(reply), 0);
            }
        }
    }

    close(listener);
    free(pfds);
}
