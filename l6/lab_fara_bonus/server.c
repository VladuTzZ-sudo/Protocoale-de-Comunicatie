/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>

#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port file\n", file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc, char **argv)
{
	int fd;

	if (argc != 3)
		usage(argv[0]);

	struct sockaddr_in my_sockaddr, from_station;
	char buf[BUFLEN];

	/*Deschidere socket*/
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	memset(&my_sockaddr, 0, sizeof(my_sockaddr));
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = htons(atoi(argv[1]));
	my_sockaddr.sin_addr.s_addr = INADDR_ANY;

	/* Legare proprietati de socket */
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
	{
		perror("setsocketopt");
		exit(1);
	}

	int rc = bind(sockfd, (struct sockaddr *)&my_sockaddr, sizeof(my_sockaddr));
	if (rc < 0)
	{
		perror("bind");
		exit(-1);
	}

	/* Deschidere fisier pentru scriere */
	DIE((fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1, "open file");

	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/

	socklen_t station_len = sizeof(from_station);
	int citire = 1;
	int count;
	while (citire > 0)
	{
		citire = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from_station, &station_len);
		count = write(fd, buf, citire);
		if (count < 0)
		{
			perror("Eroare la scriere");
			exit(-1);
		}
	}

	/*Inchidere socket*/
	close(sockfd);
	/*Inchidere fisier*/
	close(fd);

	return 0;
}
