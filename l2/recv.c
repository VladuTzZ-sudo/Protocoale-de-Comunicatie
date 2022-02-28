#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc,char** argv){
  msg r;
  init(HOST,PORT);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }
  printf("[recv] Got msg with payload: <%s>, sending ACK...\n", r.payload);
  strcat(r.payload, ".bk");
  char *file_name = strdup(r.payload);

  // Send ACK:
  sprintf(r.payload,"%s", "ACK");
  r.len = strlen(r.payload) + 1;
  send_message(&r);
  printf("[recv] ACK sent\n");


  int new_file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (new_file < 0) {
    perror("nu merge, mars");
    exit(-1);
  }
  lseek(new_file, 0, SEEK_SET);

  while(r.len != 0) {
    if (recv_message(&r)<0) {
      perror("Receive message");
      return -1;
    }
    printf("[recv] Got msg with payload: <%s>, sending ACK...\n", r.payload);
    int check = write(new_file, r.payload, r.len);
    if (check < 0) {
      perror("nu merge, mars");
      return -1;
    }
    sprintf(r.payload,"%s", "ACK");
    r.len = strlen(r.payload) + 1;
    send_message(&r);
    printf("[recv] ACK sent\n");
  }
  close(new_file);
  return 0;
}
