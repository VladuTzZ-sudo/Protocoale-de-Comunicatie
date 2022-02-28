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
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

void closeConnection(int i, fd_set read_fds, fd_set tmp_fds, int sockfd)
{
	// Se verifica daca comanda primita este "exit", in caz afirmativ
	// se deconecteaza acest client.

	FD_CLR(i, &read_fds);
	FD_CLR(i, &tmp_fds);
	FD_CLR(sockfd, &read_fds);
	FD_CLR(sockfd, &tmp_fds);

	close(sockfd);
}

void subscribe(char *buffer, int sockfd)
{
	// In caz ca mesajul este de tip "subscribe".
	// Se construieste mesajul de tip TCP, prin identificarea
	// elementelor din buffer.

	char *topic = (char *)malloc(50);
	messageTCP messageT;
	int len = strlen(buffer);
	len--;
	memset(&messageT, 0, sizeof(messageTCP));
	messageT.type = SUBSCRIBE;
	messageT.SF = atoi(&buffer[len - 2]);
	memcpy(topic, &buffer[10], (len - 12) * sizeof(char));
	strncpy(messageT.topic, topic, 50);

	// Se incearca dezactivarea algoritmului lui Neagle.
	int enable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

	// Se trimite mesajul catre server.
	int ret = send(sockfd, &messageT, sizeof(messageTCP), 0);
	DIE(ret < 0, "ERROR: sent\n");
	printf("Subscribed to topic.\n");
}

void unsubscribe(char *buffer, int sockfd)
{
	// In caz ca mesajul este de tip "unsubscribe".
	// Se construieste mesajul de tip TCP, analog.
	char *topic = (char *)malloc(50);
	messageTCP messageT;
	int len = strlen(buffer);
	len--;
	memcpy(topic, &buffer[12], (len - 14) * sizeof(char));

	messageT.type = UNSUBSCRIBE;
	messageT.SF = 2;

	strcpy(messageT.topic, topic);

	// Se incearca dezactivarea algoritmului lui Neagle.
	int enable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

	// Se trimite mesajul catre server.
	int ret = send(sockfd, &messageT, sizeof(messageTCP), 0);
	DIE(ret < 0, "ERROR: sent\n");

	printf("Unsubscribed from topic.\n");
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	int len, portno;
	struct messageUDP messageU;

	// Se declara multimile de descriptori.
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	// Se sterge in intregime multimea de descriptori de fisiere read_fds | tmp_fds.
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Este necesar un numar minim de 4 argumente.
	if (argc < 4)
	{
		usage(argv[0]);
	}

	// Portul pe care se va conecta subscriber-ul.
	portno = atoi(argv[3]);

	// Se foloseste functia socket() pentru a obtine descriptorul de fisier
	// corespunzator unei conexiuni TCP.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "ERROR: socket\n");

	// Se configureaza datele server-ului.
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "ERROR: inet_aton\n");

	// Se incearca conectarea pe acest acest server.
	ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "ERROR: connect\n");

	// Adauga in multimea read_fds descriptorii de fisiere.
	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = sockfd;

	// Se trimite ID-ul clientului la server.
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "ERROR: send\n");

	while (1)
	{
		tmp_fds = read_fds;

		// Se selecteaza descriptorul de fisiere pentru care se primesc date.
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "ERROR: select\n");

		for (int i = 0; i <= fdmax; i++)
		{
			// Se testeaza daca descriptorul i apartine sau nu multimii tmp_fds.
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == STDIN_FILENO)
				{
					// In caz ca se primesc date de la STDIN.
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					len = strlen(buffer) - 1;

					if (strncmp(buffer, "exit", 4) == 0)
					{
						// Se verifica daca comanda primita este "exit", in caz afirmativ
						// se deconecteaza acest client.
						closeConnection(i, read_fds, tmp_fds, sockfd);
						
						return 0;
					}
					if (strncmp(buffer, "subscribe", 9) == 0)
					{
						subscribe(buffer, sockfd);
					}
					else
					{
						if (strncmp(buffer, "unsubscribe", 11) == 0)
						{
							unsubscribe(buffer, sockfd);
						}
						else
						{
							printf("Invalid command !\n");
						}
					}
				}
				else
				{
					if (i == sockfd)
					{
						// In caz ca se primesc date de la server. (Mesaj UDP)
						memset(&messageU, 0, sizeof(messageUDP));
						ret = recv(sockfd, &messageU, sizeof(messageU), 0);
						DIE(ret < 0, "ERROR: recv\n");

						if (ret == 0)
						{
							// Se inchide conexiunea pentru acest client.
							closeConnection(i, read_fds, tmp_fds, sockfd);
							
							return 0;
						}

						if (strncmp(messageU.topic, "inUse", 5) == 0)
						{
							// In cazul in care exista deja un client cu acest ID,
							// atunci se va inchide conexiunea.
							closeConnection(i, read_fds, tmp_fds, sockfd);

							return 0;
						}
						else
						{
							uint32_t InT;
							printf("%s - ", messageU.topic);
							char sign;
							double ShorT, FloaT;
							uint32_t modul;
							uint8_t modulNegativ;
							switch (messageU.typeDate)
							{
							case 0:
								//Caz INT

								sign = messageU.buffer[0];
								InT = ntohl((*(uint32_t *)(messageU.buffer + 1)));
								if (sign == 1)
								{
									InT = -InT;
								}
								printf("INT - %d\n", InT);
								break;
							case 1:
								//Caz SHORT

								ShorT = ntohs((*(uint16_t *)(messageU.buffer)));
								ShorT /= 100;
								printf("SHORT_REAL - %.2lf\n", ShorT);
								break;
							case 2:
								//Caz FLOAT

								printf("FLOAT - ");
								sign = messageU.buffer[0];
								modul = ntohl((*(uint32_t *)(messageU.buffer + 1)));
								modulNegativ = (uint8_t)(messageU.buffer[5]);
								double putere = 1;
								for (int i = 0; i < modulNegativ; i++)
								{
									putere = putere * 10;
								}
								FloaT = (double)(modul / putere);
								if (sign == 1)
								{
									FloaT = -FloaT;
								}
								printf("%.16g\n", FloaT);
								break;
							case 3:
								//Caz STRING

								printf("STRING - %s\n", messageU.buffer);
								break;
							}
						}
					}
				}
			}
		}
	}

	// Se inchid socket-ul.
	close(sockfd);
	return 0;
}
