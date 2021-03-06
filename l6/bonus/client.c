/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
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
	fprintf(stderr, "Usage: %s ip_server port_server file\n", file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc, char **argv)
{

	int fd;
	struct sockaddr_in to_station;
	char buf[BUFLEN];

	/*Deschidere socket*/
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	/* Deschidere fisier pentru citire */

	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	memset(&to_station, 0, sizeof(to_station));
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &to_station.sin_addr.s_addr);

	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/
	int nrfisiere = argc - 3;
	int x = sendto(sockfd, &nrfisiere, sizeof(int), 0, (struct sockaddr *)&to_station, sizeof(to_station));
	for (int i = 0; i < nrfisiere; i++)
	{
		int scriere;
		int count = 1;
		int a = sendto(sockfd, argv[i + 3], sizeof(argv[i + 3]), 0, (struct sockaddr *)&to_station, sizeof(to_station));
		DIE((fd = open(argv[i + 3], O_RDONLY)) == -1, "open file");
		while (count > 0)
		{
			count = read(fd, buf, sizeof(buf));
			if (count < 0)
			{
				perror("Eroare la citire");
				exit(-1);
			}
			scriere = sendto(sockfd, buf, count, 0, (struct sockaddr *)&to_station, sizeof(to_station));
		}
		/*Inchidere fisier*/
		close(fd);
	}

	/*Inchidere socket*/
	close(sockfd);


	return 0;
}
