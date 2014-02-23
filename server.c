#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#define BACKLOG 10
#define BUFSIZE 1024

#define HOST_LOOKUP_CMD "ifconfig | grep -P 'inet (?!127.0.0.1)'"

void *listen_to_client();
void terminate_connection(int socketFD);

typedef struct threadArgs {
	int socketFD;
} threadArgs;

int main(int argc, char *argv[]) {
	printf("Initiating Server...\n\n");
	
	// struct servent *serv;
	struct sockaddr_in addr;
	
	// Optional specification of port number
	int port_number;
	if (argc == 2) {
		port_number = atoi(argv[1]);
	}
	else {
		// Obtain http service
		struct servent *serv  = getservbyname("http", "tcp");
		port_number = htons(serv->s_port);
		endservent();
	}
	
	// Create socket
	int sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_listen < 0) {
		printf("Failed to create listening socket\n");
		return 0;
	}
	else {
		printf("Created socket with file descriptor: %d\n", sock_listen);
	}
	
	// Setup the address (port number) to bind to
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port_number;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	// Bind the socket to the address
	if (bind(sock_listen, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		printf("Successfully bound socket to port %d (%d)\n", ntohs(addr.sin_port), addr.sin_port);
	}
	else {
		printf("Failed to bind socket to port %d\n", ntohs(addr.sin_port));
		close(sock_listen);
		return 0;
	}
	
	// Listen on socket
	if (listen(sock_listen, BACKLOG) == 0) {
		printf("Listening on socket %d\n", sock_listen);

		// Get current hosting ip address
		FILE *ptr = popen(HOST_LOOKUP_CMD, "r");
		if (ptr != NULL) {
			printf("Host(s): ");

			char buf[BUFSIZE];
			char *success;
			do {
				memset(buf, 0, BUFSIZE);
				success = fgets(buf, BUFSIZE, ptr);
				if (buf != NULL) {
					printf("%s", buf);
				}
			} while(success != NULL);
		}
	}
	else {
		printf("Failed listen on socket %d\n", sock_listen);
		close(sock_listen);
		return 0;
	}
	
	struct sockaddr client_address;
	socklen_t address_len;
	
	// Loop for accepting connections
	while (1) {
		printf("\nWaiting for connection from client...\n");
		int sock_accept = accept(sock_listen, &client_address, &address_len);
		if (sock_accept == -1) {
			printf("Failed to accept connection on listening socket %d\n", sock_listen);
		}
		else {
			printf("Accepted new connection. Created socket with file descriptor: %d\n", sock_accept);

			pthread_t newThread;
			threadArgs *args = malloc(sizeof(threadArgs));
			args->socketFD = sock_accept;

			if (pthread_create(&newThread, NULL, listen_to_client, (void *) args)) {
				printf("Failed to create thread for connection with file descriptor: %d\n", sock_accept);
			}
		}
	}
	
	close(sock_listen);
	return 0;
}

void *listen_to_client(void *tempArgs) {
	char command_buffer[BUFSIZE];
	int bytes_recieved;

	threadArgs *args = (threadArgs *) tempArgs;

	while(1) {
		printf("Waiting to receive data from client...\n");
		memset(command_buffer, 0, BUFSIZE);
		bytes_recieved = recv(args->socketFD, command_buffer, BUFSIZE, 0);
		if (bytes_recieved < 1) {
			printf("Client has closed connection\n");
			break;
		}
		else {
			printf("Data received: %s\n", command_buffer);
		}
	}

	terminate_connection(args->socketFD);
	free(args);
	pthread_exit(NULL);
}

void terminate_connection(int socketFD) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", socketFD);
	close(socketFD);
}