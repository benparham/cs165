#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define DEFAULT_TARGET_ADDRESS "127.0.0.1"
#define MAX_INPUT 1024

int main(int argc, char *argv[]) {
	printf("Initiating Client...\n");
	
	struct servent *serv;
	struct sockaddr_in addr;
	char *target_address;
	
	// Get address to connect to
	if (argc == 2) {
		target_address = argv[1];
	}
	else {
		target_address = DEFAULT_TARGET_ADDRESS;
	}
	
	// Obtain http service
	serv = getservbyname("http", "tcp");
	
	// Create socket
	int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD == -1) {
		printf("Failed to create socket\n");
		return 0;
	}
	else {
		printf("Created socket with file descriptor: %d\n", socketFD);
	}
	
	// Setup address to connect to
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv->s_port);
	endservent();	// closes etc/services
	if (inet_pton(AF_INET, target_address, &addr.sin_addr) != 1) {
		printf("Failed to convert string %s to a network address\n", target_address);
		close(socketFD);
		return 0;
	}
	
	// Connect to address
	printf("Attempting to connect to address %s...\n", target_address);
	if (connect(socketFD, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		printf("Connected to address %s\n", target_address);
	}
	else {
		printf("Failed to connect to address %s\n", target_address);
		close(socketFD);
		return 0;
	}
	
	char *input = malloc(sizeof(char) * MAX_INPUT);
	char *response = malloc(sizeof(char) * MAX_INPUT);
	
	while(1) {
		memset(input, 0, sizeof(input));
		memset(response, 0, sizeof(response));
	
		printf("Input string to send to server: ");
		fgets(input, MAX_INPUT, stdin);
		// if (strcmp(input, "exit\n") == 0) {
		// 	break;	
		// }
		// else {
			send(socketFD, input, MAX_INPUT, 0);
			/*
			int bytes_recieved = recv(socketFD, response, MAX_INPUT, 0);
			if (bytes_recieved > 0) {
				printf("Recieved response \"%s\" from server\n", response);
			}
			else {
				printf("Error recieving response from server\n");
			}*/

			if (strcmp(input, "exit\n") == 0) {
				break;	
			}
		// }
	}
	
	cleanup_socket:
	close(socketFD);
	printf("Closed socket %d\n", socketFD);
	
	free(input);
	free(response);
	return 0;
}