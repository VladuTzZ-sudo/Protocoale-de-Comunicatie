#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <sys/select.h>
#include <netinet/tcp.h>

void usage(char *file)
{
    fprintf(stderr, "Usage: %s <IP_SERVER> <PORT_SERVER>\n", file);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];

    if (argc < 3) {
        usage(argv[0]);
    }
    fd_set my_set;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    ret = inet_aton(argv[1], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    FD_ZERO(&my_set);
    FD_SET(STDIN_FILENO, &my_set);
    FD_SET(sockfd, &my_set);
    int fd_max = STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd;

    while (1) {
          // se citeste de la tastatura
        memset(buffer, 0, BUFLEN);
        fd_set tmp = my_set;

        int rc = select(fd_max + 1, &tmp, NULL, NULL, NULL);
        if (rc < 0) {
            perror("error");
            continue;
        }

        if(FD_ISSET(STDIN_FILENO, &tmp)) {
            fgets(buffer, BUFLEN, stdin);
            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }
            // se trimite mesaj la server
            n = send(sockfd, buffer, strlen(buffer), 0);
            DIE(n < 0, "send");
        } else {
            int op = recv(sockfd, buffer, BUFLEN, 0);
            if (op == 0)
                break;
                
            DIE(op < 0, "Error");
            printf("Received: %s\n", buffer);
        }
    }

    close(sockfd);

    return 0;
}