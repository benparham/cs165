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

#include "server.h"
#include "dberror.h"
#include "database.h"
#include "command.h"


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
	printf("Ready for client connections...\n");
	while (1) {
		// printf("\nWaiting for connection from client...\n");
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

	// Cast args to proper struct
	threadArgs *args = (threadArgs *) tempArgs;

	// Allocate command and error structs
	error *err = (error *) malloc(sizeof(error));
	command *cmd = createCommand();

	// The thread holds one valid table struct at a time with all table information
	dbTableInfo *currentTable = malloc(sizeof(dbTableInfo));
	currentTable->isValid = 0;

	// Begin command loop
	int done = 0;
	while (!done) {
		if (!currentTable->isValid) {
			done = requireTable(currentTable, args->socketFD, cmd, err);
		} else {
			if (receiveCommand(args->socketFD, cmd, err)) {
				done = handleReceiveErrors(err);
			} else {
				if (executeCommand(currentTable, cmd, err)) {
					done = handleExecuteErrors(err);
				}
			}
		}
	}

	// Terminate connection from server end
	terminateConnection(args->socketFD);

	// Cleanup
	free(args);
	free(err);
	destroyCommand(cmd);//free(cmd);
	free(currentTable);

	pthread_exit(NULL);
}

void terminateConnection(int socketFD) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", socketFD);
	close(socketFD);
}

// Returns 1 if error requires termination of client's thread, 0 if succeeded in setting table
int requireTable(dbTableInfo *tbl, int socketFD, command *cmd, error *err) {
	int result = 0;

	CMD cmds[3] = {CMD_USE, CMD_CREATE_TABLE, CMD_REMOVE_TABLE};
	CMD_LIST req_cmds = {cmds, 3};

	int done = 0;
	while (!done) {
		if (requireCommand(&req_cmds, socketFD, cmd, err)) {
			done = handleReceiveErrors(err);
			result = done;
		} else {
			if (cmd->cmd == CMD_EXIT) {
				printf("Exiting...\n");
				done = 1;
				result = 1;
			} else {
				if (executeCommand(tbl, cmd, err)) {
					done = handleExecuteErrors(err);
					result = done;
				} else {
					done = (tbl->name != NULL);
				}

			}
		}
	}

	return result;
}