#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

// Command functions

int parseCommand(char *buf, command *cmd, error *err) {
	if (strncmp(buf, "use ", 4) == 0) {
		cmd->cmd = CMD_USE;
		cmd->args = strtok(&buf[4], "\n");
	} else if (strncmp(buf, "select(", 7) == 0) {
		cmd->cmd = CMD_SELECT;
		cmd->args = strtok(&buf[7], ")");
	} else if (strcmp(buf, "exit\n") == 0) {
		cmd->cmd = CMD_EXIT;
		cmd->args = "";
	} else {
		err->err = ERR_INVALID_CMD;
		err->message = "Unknown command";
		return 1;
	}

	return 0;
}

int receiveCommand(int socketFD, command *cmd, error *err) {
	char buf[BUFSIZE];
	int bytesRecieved;

	printf("Waiting to receive command from client...\n");
	memset(buf, 0, BUFSIZE);

	bytesRecieved = recv(socketFD, buf, BUFSIZE, 0);
	if (bytesRecieved < 1) {
		err->err = ERR_CLIENT_EXIT;
		err->message = "Client has closed connection";
		return 1;
	}

	return parseCommand(buf, cmd, err);
}

/*
 * If receives the required command OR graceful exit, returns success.
 * Any error returns failure.
 * Else continues to ask for required command.
 */
int requireCommand(CMD req_cmd, int socketFD, command *cmd, error *err) {
	while (1) {
		if (receiveCommand(socketFD, cmd, err)) {
			return 1;
		} else {
			if (cmd->cmd == req_cmd || cmd->cmd == CMD_EXIT) {
				return 0;
			} else {
				printf("Invalid command\n");
			}
		}
	}
}