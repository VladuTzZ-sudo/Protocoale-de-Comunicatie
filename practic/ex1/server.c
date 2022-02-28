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

typedef struct client
{
	int socket;
	int noreq;
	int connected;
} client;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int isPrime(int n)
{
	// Corner case
	if (n <= 1)
	{
		return 0;
	}

	// Check from 2 to n-1
	for (int i = 2; i < n; i++)
		if (n % i == 0)
			return 0;

	return 1;
}

int checklogin(char *username, char *password)
{
	if (strcmp(username, "dinu") == 0 && strcmp(password, "dinu") == 0)
		return 1;
	if (strcmp(username, "student") == 0 && strcmp(password, "student") == 0)
		return 1;
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	char comanda[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds; // multimea de citire folosita in select()
	fd_set tmp_fds;	 // multime folosita temporar
	int fdmax;		 // valoare maxima fd din multimea read_fds

	client clients[100];
	int no_clients = 0;

	if (argc < 2)
	{
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
	{
		perror("setsocketopt\n");
		exit(1);
	}

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	while (1)
	{
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == sockfd)
				{
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);

					if (newsockfd > fdmax)
					{
						fdmax = newsockfd;
					}

					printf("Noua conexiune de la %s, port %d, socket client %d\n",
						   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
					client c;
					c.socket = newsockfd;
					c.noreq = 0;
					clients[newsockfd] = c;
					no_clients++;

					for (int j = 4; j < 4 + no_clients; j++)
					{
						printf("%d %d\n", clients[j].socket, clients[j].noreq);
					}
				}
				else
				{
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(comanda, 0, BUFLEN);
					n = recv(i, comanda, sizeof(comanda), 0);
					DIE(n < 0, "recv");

					if (n == 0)
					{
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						clients[i].connected = 0;
						no_clients--;
						close(i);
						// se scoate din multimea de citire socketul inchis
						FD_CLR(i, &read_fds);
					}
					else
					{
						if (strncmp(comanda, "login", 5) == 0)
						{
							char login[10];
							char username[50];
							char password[50];
							sscanf(buffer, "%s%s%s", login, username, password);

							if (checklogin(username, password))
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Ai reusit sa te conectezi pe contul:\nusername:%s\npassword:%s", username, password);
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
								clients[i].connected = 1;
							}
							else
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Nu ai reusit sa te conectezi, credentiale gresite:\nusername:%s\npassword:%s", username, password);
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}
						}
						else if (strncmp(comanda, "verify", 6) == 0)
						{
							char verify[10];
							int nr;
							sscanf(buffer, "%s%d", verify, &nr);
							if (clients[i].connected == 1)
							{
								if (isPrime(nr))
								{
									memset(buffer, 0, BUFLEN);
									sprintf(buffer, "Numarul %d: este prim!", nr);
									n = send(i, buffer, strlen(buffer), 0);
									DIE(n < 0, "send");
								}
								else
								{
									memset(buffer, 0, BUFLEN);
									sprintf(buffer, "Numarul %d: nu este prim!", nr);
									n = send(i, buffer, strlen(buffer), 0);
									DIE(n < 0, "send");
								}
								clients[i].noreq++;
							}
							else
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Nu ai dreptul pentru aceasta comanda, nu esti conectat!");
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}
						}
						else if (strncmp(comanda, "history", 7) == 0)
						{
							if (clients[i].connected == 1)
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Ai realizat un numar de %d verificari!", clients[i].noreq);
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}
							else
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Nu ai dreptul pentru aceasta comanda, nu esti conectat!");
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}
						}
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "Comanda incorecta!");
							n = send(i, buffer, strlen(buffer), 0);
							DIE(n < 0, "send");
						}
						printf("S-a primit de la clientul de pe socketul %d mesajul: %s", i, comanda);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}