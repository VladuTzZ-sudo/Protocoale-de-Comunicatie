# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 8080

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1 

all: server subscriber

# Compileaza server.c
server: server.c
	gcc server.c -o server -lm -g

# Compileaza client.c
subscriber: subscriber.c
	gcc subscriber.c -o subscriber -g


.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
