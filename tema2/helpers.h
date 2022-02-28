#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define DIE(assertion, call_description)  \
	do                                    \
	{                                     \
		if (assertion)                    \
		{                                 \
			fprintf(stderr, "(%s, %d): ", \
					__FILE__, __LINE__);  \
			perror(call_description);     \
			exit(EXIT_FAILURE);           \
		}                                 \
	} while (0)

#define BUFLEN 1576	  // dimensiunea maxima a calupului de date
#define MAX_CLIENTS 5 // numarul maxim de clienti in asteptare
#define SUBSCRIBE 1
#define UNSUBSCRIBE 0

typedef struct messageUDP
{
	char topic[50];
	char typeDate;
	char buffer[1500];
	char ip[20];
	int port;
} messageUDP;

typedef struct messageTCP
{
	int type;
	char topic[50];
	int SF;
} messageTCP;

typedef struct ClientTCP
{
	char ID[10];
	int connected;
	int socket;
	int messagesLength;
	struct messageUDP *waitingMessages;
} ClientTCP;

typedef struct ListClients{
	ClientTCP *client;
	int SF;
	struct ListClients *next;
} ListClients;

typedef struct Topics{
	char name[50];
	struct ListClients *clients;
} Topics;

#endif
