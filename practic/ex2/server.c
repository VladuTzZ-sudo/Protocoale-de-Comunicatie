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
	int encrypt;
	int Key;
	int decrypt;
	int connected;
} client;

typedef struct credentiale
{
	char *username;
	char *password;
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
			return 1;
		}
	}
	return 0;
}

void encrypt(char *s, int k)
{
	for (int i = 0; i < strlen(s); i++)
	{
		s[i] += k;
	}
}

void decrypt(char *s, int k)
{
	for (int i = 0; i < strlen(s); i++)
	{
		s[i] -= k;
	}
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN + 100];
	char comanda[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds; // multimea de citire folosita in select()
	fd_set tmp_fds;	 // multime folosita temporar
	int fdmax;		 // valoare maxima fd din multimea read_fds

	client clients[100];
	credentiale credite[100];
	credite[0].username = "Vlad";
	credite[0].password = "Matei";

	credite[1].username = "student";
	credite[1].password = "student";
	int no_credite = 2;
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
					c.encrypt = 0;
					c.decrypt = 0;
					c.Key = 0;
					clients[newsockfd] = c;
					no_clients++;

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
						clients[i].decrypt = 0;
						clients[i].encrypt = 0;
						clients[i].Key = 0;
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
							sscanf(comanda, "%s%s%s", login, username, password);
							printf("%s\n", username);
							printf("%s\n", password);
							if (auth(credite, no_credite, username, password))
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
								sprintf(buffer, "Nu ai reusit sa te conectezi, credentiale gresite:\nusername:%s\npassword:%s", "gresit", "gresit");
								n = send(i, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}
						}
						else if (strncmp(comanda, "encrypt", 7) == 0)
						{
							char text[BUFLEN];
							strcpy(text, comanda + strlen("encrypt "));
							if (clients[i].connected == 1)
							{
								encrypt(text, clients[i].Key);
								clients[i].encrypt++;
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Mesaj criptat: %s", text);
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
						else if (strncmp(comanda, "decrypt", 7) == 0)
						{
							char text[BUFLEN];
							strcpy(text, comanda + strlen("decrypt "));
							if (clients[i].connected == 1)
							{
								decrypt(text, clients[i].Key);
								clients[i].encrypt++;
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Mesaj decriptat: %s", text);
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
						else if (strncmp(comanda, "status", 6) == 0)
						{
							if (clients[i].connected == 1)
							{
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "Operatii criptare:%d\n Operatii decriptare:%d\n KEY:%d\n", clients[i].encrypt, clients[i].decrypt, clients[i].Key);
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
						else if (strncmp(comanda, "key", 3) == 0)
						{
							if (clients[i].connected == 1)
							{
								char login[10];
								int key;
								sscanf(comanda, "%s%d", login, &key);
								clients[i].Key = key;
								memset(buffer, 0, BUFLEN);
								sscanf(buffer, "Key actualizata!");
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