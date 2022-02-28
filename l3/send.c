#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

char control_sum(char payload[1400]) {
	char sum = 0;

	for (int i = 0; i < 1399; i++) {
		// printf("lololololo\n");
		sum ^= payload[i];
	}
	return sum;
}

int main(int argc, char *argv[])
{
	msg t;
	int i, res;
	
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	/* printf("[SENDER]: BDP=%d\n", atoi(argv[1])); */

	int BDP = atoi(argv[1]);
	int window = (BDP * 1000) / (1400 * 8);
	// *BONUS*
	//int window = 200;

	for (i = 0; i < window; i++) {
		memset(&t, 0, sizeof(msg));
		t.len = MSGSIZE;
		t.payload[1399] = control_sum(t.payload);
		res = send_message(&t);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}

	// int aux = 0;
	
	for (i = 0; i < COUNT - window; i++) {

		/* wait for ACK */
		// if (aux < window) {
			res = recv_message(&t);
			if (res < 0) {
				perror("[SENDER] Receive error. Exiting.\n");
				return -1;
			}
		// 	aux++;
		// }

		/* cleanup msg */
		memset(&t, 0, sizeof(msg));
		
		/* gonna send an empty msg */
		t.len = MSGSIZE;
		
		/* send msg */
		res = send_message(&t);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}

	for (i = 0; i < window; i++) {
		res = recv_message(&t);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
	}

	printf("[SENDER] Job done, all sent.\n");
		
	return 0;
}
