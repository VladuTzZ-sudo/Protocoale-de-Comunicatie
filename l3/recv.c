#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


char control_sum(char payload[1400]) {
	char sum = 0;

	for (int i = 0; i < 1399; i++) {
		// printf("lololololo\n");
		sum ^= payload[i];
	}
	return sum;
}

int main(void)
{
	msg r;
	int i, res;
	
	int correct = 0; 
	int not_correct = 0;

	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	for (i = 0; i < COUNT; i++) {
		/* wait for message */
		res = recv_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}
		char check = control_sum(r.payload);
		
		if (check == r.payload[1399])
			correct++;
		else
			not_correct++;

		/* send dummy ACK */
		res = send_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}
	}

	printf("[RECEIVER] Finished receiving..\n");
	printf("Correct received: %d\nNot correct: %d\n", correct, not_correct);
	printf("%.2f la suta \n", ((float)(correct) / (float)(correct + not_correct)) * 100);
	return 0;
}
