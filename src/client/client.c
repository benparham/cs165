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
#include <assert.h>

#include <message.h>

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
	endservent();	 // closes etc/services
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

	printf("\n");
	
	// Detect whether we are in interactive mode
	int inAtty = isatty(0);
	// int outAtty = isatty(1);

	char *input = malloc(sizeof(char) * MAX_INPUT);
	char *response = malloc(sizeof(char) * MAX_INPUT);
	
	while(1) {
		memset(input, 0, sizeof(input));
		memset(response, 0, sizeof(response));
		
		// Prompt/get input
		printf(">: ");
		fgets(input, MAX_INPUT, stdin);

		// Display input if not in interactive mode
		if (!inAtty) {
			printf("%s", input);
		}

		if (messageSend(socketFD, input)) {
			printf("->: Error sending message\n");
		}

		if (strcmp(input, "exit\n") == 0) {
			break;
		}

		int dataBytes;
		void *data;
		int term;
		if (messageReceive(socketFD, response, &dataBytes, &data, &term)) {
			if (term) {
				printf("->: Server has exited\n");
				break;
			} else {
				printf("->: Error receiving message\n");
			}
		}


		printf("->: %s\n", response);

		if (dataBytes > 0) {
			
			assert(dataBytes % sizeof(int) == 0);
			int nEntries = dataBytes / sizeof(int);

			printf("->: [");

			for (int i = 0; i < nEntries; i++) {
				
				// char *entry;
				// sprintf(entry, "%d", ((int *) data)[i]);

				// printf("%s", entry);
				printf("%d", ((int *) data)[i]);


				if (i != nEntries - 1) {
					printf(",");
				}
			}

			printf("]\n");

			free(data);
		}
	}
	
	// Cleanup socket
	close(socketFD);
	printf("Closed socket %d\n", socketFD);
	
	free(input);
	free(response);
	return 0;
}