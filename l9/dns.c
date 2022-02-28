// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int usage(char* name)
{
	printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
	return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	// TODO: set hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_socktype = SOCK_STREAM;

	// TODO: get addresses
	int rc = getaddrinfo(name, NULL, &hints, &result);
	if(rc != 0) {
    	printf("%s", gai_strerror(rc));
	}
	// TODO: iterate through addresses and print them
	p = result;
	while(p) {
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
	        char addr1[INET_ADDRSTRLEN];
    	    inet_ntop(p->ai_family, &addr->sin_addr, addr1, sizeof(addr1));
			printf("%s\n", addr1);
		} else if (p->ai_family == AF_INET6) {
			struct sockaddr_in6 *addr = (struct sockaddr_in6 *)p->ai_addr;
	        char addr1[INET6_ADDRSTRLEN];
    	    inet_ntop(p->ai_family, &addr->sin6_addr, addr1, sizeof(addr1));
			printf("%s\n", addr1);
		}
		p = p->ai_next;
	}
	// TODO: free allocated data
	freeaddrinfo(result);

}

// Receives an address and prints the associated name and service
void get_name(char* ip)
{
	int ret;
	struct sockaddr_in addr;
	char host[1024];
	char service[20];

	memset(&addr, 0, sizeof(addr));
	// TODO: fill in address data
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	inet_aton(ip, &addr.sin_addr);
	// TODO: get name and service
	ret = getnameinfo((struct sockaddr *)&addr, sizeof(addr), host, 1024, service, 20,  0);
	if(ret != 0) {
    	printf("%s", gai_strerror(ret));
	}
	// TODO: print name and service
	printf("%s %s\n", host, service);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return usage(argv[0]);
	}

	if (strncmp(argv[1], "-n", 2) == 0) {
		get_ip(argv[2]);
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		get_name(argv[2]);
	} else {
		return usage(argv[0]);
	}

	return 0;
}
