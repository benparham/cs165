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
#include <assert.h>

#include "database.h"

#define BACKLOG 				10

#define HOST_LOOKUP_CMD "ifconfig | grep -P 'inet (?!127.0.0.1)'"

void *listenToClient();
void terminateConnection(int socketFD);
int cmdUse(int socketFD, FILE *fp);
int openTable(char *tableName, FILE *fp);

typedef struct threadArgs {
	int socketFD;
} threadArgs;

int main(int argc, char *argv[]) {
	printf("Initiating Server...\n\n");
	
	// struct servent *serv;
	struct sockaddr_in addr;
	
	// Optional specification of port number
	int portNumber;
	if (argc == 2) {
		portNumber = atoi(argv[1]);
	}
	else {
		// Obtain http service
		struct servent *serv  = getservbyname("http", "tcp");
		portNumber = htons(serv->s_port);
		endservent();
	}
	
	// Create socket
	int sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockListen < 0) {
		printf("Failed to create listening socket\n");
		return 0;
	}
	else {
		printf("Created socket with file descriptor: %d\n", sockListen);
	}
	
	// Setup the address (port number) to bind to
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = portNumber;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	// Bind the socket to the address
	if (bind(sockListen, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		printf("Successfully bound socket to port %d (%d)\n", ntohs(addr.sin_port), addr.sin_port);
	}
	else {
		printf("Failed to bind socket to port %d\n", ntohs(addr.sin_port));
		close(sockListen);
		return 0;
	}
	
	// Listen on socket
	if (listen(sockListen, BACKLOG) == 0) {
		printf("Listening on socket %d\n", sockListen);

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
		printf("Failed listen on socket %d\n", sockListen);
		close(sockListen);
		return 0;
	}
	
	struct sockaddr clientAddress;
	socklen_t addressLen;
	
	// Loop for accepting connections
	while (1) {
		printf("\nWaiting for connection from client...\n");
		int sockAccept = accept(sockListen, &clientAddress, &addressLen);
		if (sockAccept == -1) {
			printf("Failed to accept connection on listening socket %d\n", sockListen);
		}
		else {
			printf("Accepted new connection. Created socket with file descriptor: %d\n", sockAccept);

			pthread_t newThread;
			threadArgs *args = malloc(sizeof(threadArgs));
			args->socketFD = sockAccept;

			if (pthread_create(&newThread, NULL, listenToClient, (void *) args)) {
				printf("Failed to create thread for connection with file descriptor: %d\n", sockAccept);
			}
		}
	}
	
	close(sockListen);
	return 0;
}

void *listenToClient(void *tempArgs) {
	// char commandBuffer[BUFSIZE];
	// int bytesRecieved;

	// Cast args to proper struct
	threadArgs *args = (threadArgs *) tempArgs;

	// Allocate command and error structs
	error *err = (error *) malloc(sizeof(error));
	command *cmd = (command *) malloc(sizeof(command));

	int done = 0;
	while (!done) {
		if (requireCommand(CMD_USE, args->socketFD, cmd, err)) {
			switch (err->err) {
				case ERR_GENERAL:
				case ERR_INVALID_CMD:
					printf("%s\n", err->message);
					break;
				case ERR_CLIENT_EXIT:
					printf("%s\n", err->message);
					done = 1;
					break;
				default:
					printf("Unknown error\n");
					break;
			}
		} else {
			if (cmd->cmd == CMD_EXIT) {
				printf("Exiting...\n");
				done = 1;
			} else {
				assert(cmd->cmd == CMD_USE);
				printf("Received command 'use' with args: %s\n", cmd->args);
			}
		}
	}

	// FILE *fp;
	// if (!cmdUse(args->socketFD, fp)) {
	// 	while(1) {
	// 		printf("Waiting to receive data from client...\n");
	// 		memset(commandBuffer, 0, BUFSIZE);
	// 		bytesRecieved = recv(args->socketFD, commandBuffer, BUFSIZE, 0);
	// 		if (bytesRecieved < 1) {
	// 			printf("Client has closed connection\n");
	// 			terminateConnection(args->socketFD);
	// 			break;
	// 		}
	// 		else {
	// 			printf("Data received: %s\n", commandBuffer);
	// 		}
	// 	}

	// 	fclose(fp);
	// }

	// Terminate connection from server end
	terminateConnection(args->socketFD);

	// Cleanup
	free(args);
	free(err);
	free(cmd);

	pthread_exit(NULL);
}

void terminateConnection(int socketFD) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", socketFD);
	close(socketFD);
}

int cmdUse(int socketFD, FILE *fp) {
	char buf[BUFSIZE];
	int bytesRecieved;

	while (1) {
		printf("Waiting to receive table from client...\n");
		memset(buf, 0, BUFSIZE);

		bytesRecieved = recv(socketFD, buf, BUFSIZE, 0);
		if (bytesRecieved < 1) {
			printf("Client has closed connection\n");
			terminateConnection(socketFD);
			return 1;
		}

		if (strncmp(buf, "use ", 4) == 0) {
			printf("Request to use table: %s\n", &buf[4]);
			if (openTable(strtok(&buf[4], "\n"), fp)) {
				continue;
			} else {
				break;
			}
		} else {
			printf("Invalid command. Require 'use <table name>'\n");
		}
	}

	return 0;
}

int openTable(char *tableName, FILE *fp) {
	char fileName[BUFSIZE];
	sprintf(fileName, "db/%s.txt", tableName);
	
	fp = fopen(fileName, "r");

	if (fp == NULL) {
		printf("Unknown table: %s\n", tableName);
		return 1;
	}

	printf("Opened table %s\n", fileName);
	return 0;
}