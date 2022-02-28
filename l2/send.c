#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;

  //Send dummy message:
  printf("[send] Sending file...\n");
  sprintf(t.payload,"%s", argv[1]);
  t.len = strlen(t.payload)+1;
  send_message(&t);
  
  // Check response:
  if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }

  int file = open(argv[1], O_RDONLY);
  if (file < 0) {
    perror("cacao");
    exit(-1);
  }
  //char buff[1400];

  while(1) {
    int line = read(file, t.payload, 666);
    if (line == 0) {
      close(file);
      printf("[send] Sending file...\n");
      sprintf(t.payload,"%c", '\n');
      t.len = 0;
      send_message(&t);    

      if (recv_message(&t)<0){
      perror("Receive error ...");
      return -1;
      }
      else {
        printf("[send] Got reply with payload: %s\n", t.payload);
      }
      break;
    }
    else if (line < 0) {
      close(file);
      perror("read line");
      exit(-1);
    }
    // else if (line > 0) {
    //   line = write(1, buff, line);
    // }
    t.len = line;
    printf("[send] Sending file...\n");
    //sprintf(t.payload,"%s", buff);
    send_message(&t);    

    if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
    }
    else {
      printf("[send] Got reply with payload: %s\n", t.payload);
    }

  }
  return 0;
}
