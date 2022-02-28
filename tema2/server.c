#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/select.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

// Functie ce cauta in lista de clienti inregistrati, clientul cu socket-ul dat.
ClientTCP *client_with_socket_i(int socketSearch, ListClients *clients)
{
	// Se parcurge lista de clienti si se intoarce clientul numai daca este conectat.
	struct ListClients *aux;
	aux = clients;
	while (aux != NULL)
	{
		if (aux->client->socket == socketSearch)
		{
			if (aux->client->connected == 1)
			{
				return aux->client;
			}
		}

		aux = aux->next;
	}
	return NULL;
}

// Functia are ca scop sa gaseasca pentru topic-ul mesajului lista de clienti abonati si
// sa le transmita mesajul sau cel putin sa puna mesajul in asteptare pana cand se vor conecta.
Topics *sendToClient(struct messageUDP *messageU, int *length, struct Topics *topics)
{
	int flag = 0;
	for (int i = 0; i < *length; i++)
	{
		if (strncmp(topics[i].name, messageU->topic, strlen(messageU->topic)) == 0)
		{
			// Daca s-a gasit topic-ul cautat in vectorul de topics, atunci:
			flag = 1;
			ListClients *clients = topics[i].clients;

			// Parcurg clientii.
			while (clients != NULL)
			{
				ClientTCP *TCPClient = clients->client;
				// Exista posibilitatea ca o lista de clienti sa nu fie NULL,
				// sa aiba memorie declarata, dar sa nu existe niciun client.
				if (TCPClient != NULL)
				{
					if (TCPClient->connected == 1)
					{
						// Deoarece clientul este conectat i se trimite mesajul.
						// Se incearca dezactivarea algoritmului lui Neagle.
						int enable = 1;
						setsockopt(TCPClient->socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
						struct messageUDP mesaj;
						mesaj = *messageU;
						send(TCPClient->socket, &mesaj, sizeof(mesaj), 0);
					}
					else
					{
						// Daca clientul este offline, atunci se verifica setarea STORE&FORWARD.
						if (clients->SF == 1)
						{
							// Se initializeaza | reinitializeaza cu memorie vectorul de mesaje aflate in asteptare.
							if (TCPClient->messagesLength == 0)
							{
								TCPClient->waitingMessages = malloc(sizeof(messageUDP));
							}
							else
							{
								TCPClient->waitingMessages = (messageUDP *)realloc(TCPClient->waitingMessages,
																				   (TCPClient->messagesLength + 2) * sizeof(messageUDP));
							}

							// Se adauga mesajul in vector.
							TCPClient->waitingMessages[TCPClient->messagesLength] = *messageU;
							TCPClient->messagesLength++;
						}
					}
					clients = clients->next;
				}
				else
				{
					break;
				}
			}
			break;
		}
	}
	if (flag == 0)
	{
		// Daca nu s-a gasit topic-ul cautat atunci se creeaza acum acest topic si se adauga in vector.
		topics = (Topics *)realloc(topics, (*length + 2) * sizeof(Topics));
		topics[*length].clients = (ListClients *)malloc(sizeof(ListClients));

		strcpy(topics[*length].name, messageU->topic);
		topics[*length].clients->client = NULL;
		topics[*length].clients->next = NULL;
		topics[*length].clients->SF = 2;
		(*length)++;
	}
	return topics;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	Topics *topics = NULL;
	int lengthTopics = 0;
	ListClients *clients = NULL;
	int socketUDP, socketTCP, portno, newsocket;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t cli_len;
	int ret, n;
	messageUDP messageU;

	// Se declara multimile de descriptori.
	fd_set read_fds; // multimea de citire folosita in select()
	fd_set tmp_fds;	 // multime folosita temporar
	int fdmax;		 // valoare maxima fd din multimea read_fds

	if (argc < 2)
	{
		usage(argv[0]);
	}

	// Se sterge in intregime multimea de descriptori de fisiere read_fds | tmp_fds.
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	portno = atoi(argv[1]);
	DIE(portno == 0, "ERROR: atoi\n");

	// Se foloseste functia socket() pentru a obtine descriptorul de fisier
	// corespunzator unei conexiuni UDP.
	socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(socketUDP < 0, "ERROR: socket UDP\n");

	// Se incearca dezactivarea algoritmului lui Neagle.
	int enable = 1;
	int setsocketOPT = setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(setsocketOPT == -1, "ERROR: Nagle's algorithm UDP\n");

	// Se foloseste functia socket() pentru a obtine descriptorul de fisier
	// corespunzator unei conexiuni TCP.
	socketTCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socketTCP < 0, "ERROR: socket TCP\n");

	// Se incearca dezactivarea algoritmului lui Neagle.
	setsocketOPT = setsockopt(socketTCP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(setsocketOPT == -1, "ERROR: Nagle's algorithm TCP\n");

	// Se configureaza datele server-ului.
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Se asociaza socket-ului UDP portul.
	ret = bind(socketUDP, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "ERROR: bind UDP\n");

	// Se asociaza socket-ului TCP portul.
	ret = bind(socketTCP, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "ERROR: bind TCP\n");

	// Socket-ul pe care se poate asculta este socket-ul corespunzator conexiunii TCP.
	ret = listen(socketTCP, MAX_CLIENTS);
	DIE(ret < 0, "ERROR: listen TCP\n");

	// Se adauga in multimea read_fds descriptorii de fisiere.
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(socketUDP, &read_fds);
	FD_SET(socketTCP, &read_fds);
	fdmax = max(socketTCP, socketUDP);

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
					memset(&buffer, 0, sizeof(buffer));
					fgets(buffer, BUFLEN, stdin);

					if (strncmp(buffer, "exit", 4) == 0)
					{
						// Se verifica daca comanda primita este "exit", in caz afirmativ
						// se deconecteaza clientii, se curata setul ce contine descriptorii.
						for (int i = 0; i <= fdmax; i++)
						{
							FD_CLR(i, &read_fds);
							close(i);
						}
						FD_ZERO(&read_fds);
						return 0;
					}
					else
					{
						printf("Invalid command !\n");
					}
				}
				else
				{
					if (i == socketUDP)
					{
						// In caz ca se primesc date pe socket-ul corespunzator conexiunii UDP.
						cli_len = sizeof(cli_addr);
						memset(&buffer, 0, sizeof(buffer));
						memset(&messageU, 0, sizeof(messageUDP));

						// Se primeste mesajul.
						ret = recvfrom(socketUDP, buffer, BUFLEN, 0, (struct sockaddr *)&cli_addr, &cli_len);

						// Se identifica elementele si se creeaza mesajul in formatul messageUDP.
						memcpy(messageU.topic, buffer, 50);
						strcpy(messageU.ip, inet_ntoa(cli_addr.sin_addr));
						if (strlen(&buffer[51]) == 1500)
						{
							strcat(buffer, "\0");
						}
						memcpy(messageU.buffer, &buffer[51], 1501);

						messageU.typeDate = buffer[50];
						messageU.port = ntohs(cli_addr.sin_port);

						// In caz ce nu exista topic-uri in vectorul de topics.
						if (topics == NULL)
						{
							// Se initializeaza vectorul de topics cu topic-ul acestui mesaj ca prim topic.
							topics = (Topics *)malloc(sizeof(Topics));
							topics[lengthTopics].clients = (ListClients *)malloc(sizeof(ListClients));

							strcpy(topics[lengthTopics].name, messageU.topic);
							topics[lengthTopics].clients->client = NULL;
							topics[lengthTopics].clients->next = NULL;
							topics[lengthTopics].clients->SF = 2;
							lengthTopics++;
						}
						else
						{
							// In caz ca exista cel putin 1 topic in vectorul de topics se apeleaza functia.
							topics = sendToClient(&messageU, &lengthTopics, topics);
						}
					}
					else
					{
						if (i == socketTCP)
						{
							// In caz ca se doreste o noua conexiune TCP.
							cli_len = sizeof(cli_addr);
							newsocket = accept(socketTCP, (struct sockaddr *)&cli_addr, &cli_len);

							DIE(newsocket < 0, "ERROR: accept\n");

							// Se adauga socket-ul clientului nou in multimea de descriptori de fisiere. 
							FD_SET(newsocket, &read_fds);
							fdmax = max(fdmax, newsocket);

							// Se incearca dezactivarea algoritmului lui Neagle.
							setsockopt(newsocket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

							char id[10];
							memset(id, 0, 10);
							n = recv(newsocket, id, sizeof(id), 0);

							DIE(n < 0, "ERROR: recv\n");

							// Se adauga clientul nou in lista de clienti.
							struct ListClients *aux;
							aux = clients;

							if (clients == NULL)
							{
								// Daca lista este goala, atunci se initializeaza cu clientul nou.
								clients = (ListClients *)malloc(sizeof(struct ListClients));
								clients->client = malloc(sizeof(struct ClientTCP));
								strcpy(clients->client->ID, id);
								clients->client->socket = newsocket;
								clients->client->connected = 1;
								clients->SF = 0;
								clients->client->messagesLength = 0;
								clients->next = NULL;

								printf("New client %s connected from %s:%d\n", id,
									   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							}
							else
							{
								// Se incearca parcurgerea listei de clienti deja conectati, adaugarea se face la sfarsit.
								int flag = 0;
								while (aux != NULL)
								{
									if (strcmp(id, aux->client->ID) == 0)
									{
										// Daca clientul nou s-a gasit in lista de clienti inseamna ca a mai fost 
										// conectat si a iesit ,dar acum se reconecteaza sau chiar acum este conectat.
										flag = 1;
										if (aux->client->connected == 1)
										{
											// Daca chiar acum este conectat trimitem catre client un mesaj "inUse"
											// ce va fi decodificat in client si printeaza in server (aici) eroarea aferenta.
											struct messageUDP newMessage;
											char notification[BUFLEN];
											memset(&newMessage, 0, sizeof(newMessage));

											strcpy(notification, "inUse");
											strcpy(newMessage.topic, notification);

											n = send(newsocket, &newMessage, sizeof(newMessage), 0);
											DIE(n < 0, "ERROR: send\n");

											close(newsocket);
											FD_CLR(newsocket, &read_fds);
											printf("Client %s already connected.\n", id);
										}
										else
										{
											// Inseamna ca acesta este un client care se reconecteaza la server.
											// Se reactualizeaza socket-ul.
											aux->client->connected = 1;
											aux->client->socket = newsocket;

											// Se cauta in listele de clienti abonati la topic-uri daca si acesta este
											// abonat pentru a i se trimite mesajele primite cat a fost offline, daca SF == 1.
											for (int i = 0; i < lengthTopics; i++)
											{
												ListClients *clientsForResend = topics[i].clients;
												while (clientsForResend != NULL)
												{
													if (clientsForResend->SF == 1)
													{
														if (strcmp(clientsForResend->client->ID, aux->client->ID) == 0)
														{
															for (int j = 0; j < clientsForResend->client->messagesLength; j++)
															{
																send(newsocket, &clientsForResend->client->waitingMessages[j], sizeof(struct messageUDP), 0);
															}
														}
													}
													clientsForResend = clientsForResend->next;
												}
											}

											// Se printeaza in server mesajul asteptat pentru o noua conexiune cu un client TCP.
											printf("New client %s connected from %s:%d\n", aux->client->ID,
												   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
										}
									}

									// Daca s-a ajuns la sfarsitul iterarii si nu s-a gasit clientul, atunci se adauga ca unul nou.
									if (aux->next == NULL && flag == 0)
									{
										struct ClientTCP *newClient = malloc(sizeof(struct ClientTCP));
										strcpy(newClient->ID, id);
										newClient->socket = newsocket;
										newClient->connected = 1;
										newClient->messagesLength = 0;
										struct ListClients *newList = malloc(sizeof(struct ListClients));
										newList->client = newClient;
										newList->SF = 0;
										newList->next = NULL;
										aux->next = newList;

										printf("New client %s connected from %s:%d\n", id,
											   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
										break;
									}
									aux = aux->next;
								}
							}
						}
						else
						{
							// Pentru unul dintre clientii TCP s-au primit date, deci se interpreteaza.
							memset(buffer, 0, BUFLEN);
							n = recv(i, buffer, sizeof(buffer), 0);
							DIE(n < 0, "ERROR: recv\n");

							if (n == 0)
							{
								// Daca n == 0, atunci inseamna ca acest client trebuie deconectat de la server.
								char *id = malloc(10);
								struct ListClients *aux;
								aux = clients;

								while (aux != NULL)
								{
									if (aux->client->socket == i)
									{
										strcpy(id, aux->client->ID);
										aux->client->connected = 0;
									}
									aux = aux->next;
								}

								printf("Client %s disconnected.\n", id);
								close(i);

								// Se scoate din multimea descriptorilor socket-ul clientului deconectat.
								FD_CLR(i, &read_fds);
							}
							else
							{
								// Altfel se astepta un mesaj de tipul SUBSCRIBE | UNSUBSCRIBE.
								struct messageTCP *messageT;
								messageT = (struct messageTCP *)buffer;

								if (lengthTopics == 0)
								{
									// Daca nu exista niciun topic inregistrat in server, atunci se poate doar subscribe.
									if (messageT->type == SUBSCRIBE)
									{
										ClientTCP *clientFound = client_with_socket_i(i, clients);
										if (clientFound != NULL)
										{
											// Se creeaza aici primul topic din server, initializandu-se cu primul client.
											topics = (Topics *)malloc(sizeof(Topics));
											topics->clients = (ListClients *)malloc(sizeof(ListClients));

											lengthTopics++;

											strcpy(topics[0].name, messageT->topic);
											topics[0].clients->next = NULL;
											topics[0].clients->SF = messageT->SF;
											topics[0].clients->client = clientFound;
										}
									}
								}
								else
								{
									int gasit = 0;
									ClientTCP *clientFound = client_with_socket_i(i, clients);

									// Se cauta printre topic-urile din server daca cel la care vrea clientul sa se aboneze exista.
									for (int j = 0; j < lengthTopics; j++)
									{
										if (strncmp(topics[j].name, messageT->topic, strlen(messageT->topic)) == 0)
										{
											// Daca topic-ul exista, atunci se verifica daca clientul realizeaza SUBSCRIBE | UNSUBSCRIBE.
											gasit = 1;
											if (clientFound != NULL)
											{
												if (messageT->type == SUBSCRIBE)
												{
													// Pentru SUBSCRIBE, se verifica daca este primul client din lista de abonati.
													int flag = 0;
													ListClients *aux = topics[j].clients;
													if (aux->client == NULL)
													{
														//Se adauga ca primul client abonat la acest topic.
														aux->next = NULL;
														aux->SF = messageT->SF;
														aux->client = clientFound;
													}
													else
													{
														// Daca nu este primul client din lista de abonati, atunci se verifica daca exista deja ca abonat.
														while (aux != NULL)
														{
															if (strcmp(aux->client->ID, clientFound->ID) == 0)
															{
																flag = 1;
																break;
															}
															if (aux->next == NULL && flag == 0)
															{
																// Pentru ca nu a fost gasit ca abonat, se adauga la sfarsitul listei ca un abonat nou.
																ListClients *newList = malloc(sizeof(ListClients));
																newList->next = NULL;
																newList->SF = messageT->SF;
																newList->client = clientFound;

																aux->next = newList;

																aux = aux->next->next;
															}
															else
															{
																aux = aux->next;
															}
														}
													}
												}
												if (messageT->type == UNSUBSCRIBE)
												{
													// Pentru UNSUBSCRIBE se parcurge lista de clienti din topic-ul gasit.
													int flag = 0;
													ListClients *temp;
													ListClients *aux = topics[j].clients;

													// Se verifica " head-ul " listei daca este clientul cautat.
													if (topics[j].clients != NULL)
													{
														if (strcmp(topics[j].clients->client->ID, clientFound->ID) == 0)
														{
															flag = 1;
															topics[j].clients = topics[j].clients->next;
														}
													}

													// Daca " head-ul " listei nu este clientul cautat atunci se parcurge lista
													// privind inainte cu un pas.
													if (flag == 0)
													{
														int flag2 = 0;
														while (aux->next != NULL && flag2 == 0)
														{
															if (strcmp(aux->next->client->ID, clientFound->ID) == 0)
															{
																// Daca urmatorul client din lista este cel cautat, se elimina.
																temp = aux->next;
																aux->next = aux->next->next;
																free(temp);
																flag2 = 1;
																break;
															}
															aux = aux->next;
														}
													}
												}
											}
										}
									}
									if (gasit == 0)
									{
										// Daca topic-ul cautat nu este gasit, atunci se reinitializeaza vectorul de topics.
										// Topic-ul nou adaugat va avea ca prim abonat acest client.
										topics = (Topics *)realloc(topics, (lengthTopics + 2) * sizeof(Topics));
										topics[lengthTopics].clients = (ListClients *)malloc(sizeof(ListClients));
										strcpy(topics[lengthTopics].name, messageT->topic);
										topics[lengthTopics].clients->next = NULL;
										topics[lengthTopics].clients->SF = messageT->SF;
										topics[lengthTopics].clients->client = clientFound;
										lengthTopics++;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Se inchid sockets.
	close(socketUDP);
	close(socketTCP);
	return 0;
}