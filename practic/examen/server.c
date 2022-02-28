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

typedef struct mesaje
{
	char *pagina;
	char *continut;
} mesaje;

typedef struct credentiale
{
	char *username;
	char *password;
	int conected;
} credentiale;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int auth(credentiale *credite, int no_credite, char *username, char *password)
{
	for (int i = 0; i < no_credite; i++)
	{
		if (strcmp(credite[i].username, username) == 0 &&
			strcmp(credite[i].password, password) == 0)
		{
			credite[i].conected = 1;
			return 1;
		}
	}
	return 0;
}

int getIndexPagina(mesaje *mesaje, int no_mesaje, char *pagina)
{
	for (int i = 0; i < no_mesaje; i++)
	{
		if (strcmp(mesaje[i].pagina, pagina) == 0)
		{
			return i;
		}
	}
	return no_mesaje;
}

int main(int argc, char *argv[])
{
	int sockfd, portno;
	char buffer[BUFLEN + 100];
	char comanda[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;

	fd_set read_fds; // multimea de citire folosita in select()
	fd_set tmp_fds;	 // multime folosita temporar
	int fdmax;		 // valoare maxima fd din multimea read_fds

	mesaje mesaje[100];
	credentiale credite[100];
	credite[0].username = "Vlad";
	credite[0].password = "Matei";
	credite[0].conected = 0;

	credite[1].username = "student";
	credite[1].password = "student";
	credite[1].conected = 0;
	int no_credite = 2;
	int no_mesaje = 0;

	if (argc < 2)
	{
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	//ret = listen(sockfd, MAX_CLIENTS);
	//DIE(ret < 0, "listen");

	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
	{
		perror("setsocketopt\n");
		exit(1);
	}

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	int connected = 0;

	socklen_t l = sizeof(cli_addr);
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
					int citire = 1;

					citire = recvfrom(sockfd, comanda, sizeof(comanda), 0, (struct sockaddr *)&cli_addr, &l);
					if (citire == -1)
					{
						printf("Eroare receive!");
						return -1;
					}

					if (strncmp(comanda, "login", 5) == 0)
					{
						char login[10];
						char username[15];
						char password[15];
						sscanf(comanda, "%s%s%s", login, username, password);
						if (auth(credite, no_credite, username, password))
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
							connected = 1;
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
					}

					if (strncmp(comanda, "PUT", 3) == 0)
					{
						char login[10];
						char nume[15];
						char continut[50];
						sscanf(comanda, "%s%s%s", login, nume, continut);
						char finalContinut[100];
						strcpy(finalContinut, continut + strlen("HTTP/1.1\r\n\r\n"));
						if (connected == 1)
						{
							int index = getIndexPagina(mesaje, no_mesaje, nume);
							if (index == no_mesaje)
							{
								no_mesaje++;
							}
							mesaje[index].continut = finalContinut;
							mesaje[index].pagina = nume;
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
					}

					if (strncmp(comanda, "GET", 3) == 0)
					{
						char login[10];
						char nume[15];
						char continut[50];
						sscanf(comanda, "%s%s%s", login, nume, continut);

						int index = getIndexPagina(mesaje, no_mesaje, nume);
						if (index == no_mesaje)
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 404 Not Found\r\n\r\n");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
							strcpy(buffer, mesaje[index].continut);
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
							
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "Pagina cautata nu exista");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
					}

					if (strncmp(comanda, "DELETE", 6) == 0)
					{
						char login[10];
						char nume[15];
						char continut[50];
						sscanf(comanda, "%s%s%s", login, nume, continut);
						if (connected == 1)
						{
							int index = getIndexPagina(mesaje, no_mesaje, nume);
							if (index == no_mesaje)
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "HTTP/1.1 404 Not Found\r\n\r\n");
								sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
							}
							else
							{
								mesaje[index].continut = "";
								mesaje[index].pagina = "";
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "HTTP/1.1 200 OK\r\n\r\n");
								sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
							}
						}
						else
						{
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "HTTP/1.1 401 Unauthorized\r\n\r\n");
							sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, l);
						}
					}
				}
				else if (i == STDIN_FILENO)
				{
					memset(&comanda, 0, sizeof(comanda));
					fgets(comanda, BUFLEN, stdin);
					DIE(n < 0, "recv");
					if (strncmp(comanda, "quit", 4) == 0)
					{
						for (int i = 0; i <= fdmax; i++)
						{
							FD_CLR(i, &read_fds);
							close(i);
						}
						sendto(sockfd, buffer, 0, 0, (struct sockaddr *)&cli_addr, l);
						FD_ZERO(&read_fds);
						return 0;
					}
					if (strncmp(comanda, "status", 6) == 0)
					{
						for (int i = 0; i < no_mesaje; i++)
						{
							printf("Mesaj: -> pagina: %s , continut: %s", mesaje[i].pagina, mesaje[i].continut);
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}