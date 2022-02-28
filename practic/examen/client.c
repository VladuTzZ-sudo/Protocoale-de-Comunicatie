/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>

#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s ip_server port_server file\n", file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc, char **argv)
{
	if (argc != 3)
		usage(argv[0]);

	int fd;
	struct sockaddr_in serv_addr;
	char buf[BUFLEN];
	fd_set my_set;

	/*Deschidere socket*/
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		printf("EROARE SOCKET.");
		return -1;
	}

	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	int ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/
	FD_ZERO(&my_set);
	FD_SET(STDIN_FILENO, &my_set);
	FD_SET(sockfd, &my_set);
	int fd_max = STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd;
	while (1)
	{
		memset(buf, 0, BUFLEN);
		fd_set tmp = my_set;

		int rc = select(fd_max + 1, &tmp, NULL, NULL, NULL);
		if (rc < 0)
		{
			perror("error");
			continue;
		}

		if (FD_ISSET(STDIN_FILENO, &tmp))
		{
			fgets(buf, BUFLEN, stdin);
			if (strncmp(buf, "quit", 4) == 0)
			{
				break;
			}
			sendto(sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		}
		else
		{
			int op = recv(sockfd, buf, BUFLEN, 0);
			if (op == 0)
				break;
			DIE(op < 0, "Error");
			printf("Received: %s\n", buf);
		}
	}
	/*Inchidere socket*/
	close(sockfd);

	/*Inchidere fisier*/
	close(fd);

	return 0;
}
