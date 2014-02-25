#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "command.h"
#include "dberror.h"

// Command functions

// Definition of global array of command strings matched to enum CMD
const char *CMD_NAMES[] = {
	"use",
	"create table",
	"select",
	"fetch",
	"create",
	"load",
	"insert",
	"exit"
};

command* createCommand() {
	command* cmd = (command *) malloc(sizeof(command));
	cmd->args = NULL;
	return cmd;
}

void commandAllocCopyArgs(command *cmd, char *args) {
	cmd->args = (char *) malloc(sizeof(args));
	strcpy(cmd->args, args);
}

void destroyCommand(command *cmd) {
	if (cmd->args != NULL) {
		free(cmd->args);
	}

	free(cmd);
}

int parseCommand(char *buf, command *cmd, error *err) {
	if (strncmp(buf, "use ", 4) == 0) {
		cmd->cmd = CMD_USE;
		commandAllocCopyArgs(cmd, strtok(&buf[4], "\n"));
	} else if (strncmp(buf, "create table ", 13) == 0) {
		cmd->cmd = CMD_CREATE_TABLE;
		commandAllocCopyArgs(cmd, strtok(&buf[13], "\n"));
	} else if (strncmp(buf, "select(", 7) == 0) {
		cmd->cmd = CMD_SELECT;
		commandAllocCopyArgs(cmd, strtok(&buf[7], ")"));
	} else if (strncmp(buf, "fetch(", 6) == 0) {
		cmd->cmd = CMD_FETCH;
		commandAllocCopyArgs(cmd, strtok(&buf[6], ")"));
	} else if (strncmp(buf, "create(", 7) == 0) {
		cmd->cmd = CMD_CREATE;
		commandAllocCopyArgs(cmd, strtok(&buf[7], ")"));
	} else if (strncmp(buf, "load(", 5) == 0) {
		cmd->cmd = CMD_LOAD;
		commandAllocCopyArgs(cmd, strtok(&buf[5], ")"));
	} else if (strncmp(buf, "insert(", 7) == 0) {
		cmd->cmd = CMD_INSERT;
		commandAllocCopyArgs(cmd, strtok(&buf[7], ")"));
	} else if (strcmp(buf, "exit\n") == 0) {
		cmd->cmd = CMD_EXIT;
		commandAllocCopyArgs(cmd, "");
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
int requireCommand(CMD_LIST *req_cmds, int socketFD, command *cmd, error *err) {
	while (1) {
		if (receiveCommand(socketFD, cmd, err)) {
			return 1;
		} else {
			if (cmd->cmd == CMD_EXIT) {
				return 0;
			}
			for (int i = 0; i < req_cmds->length; i++) {
				if (cmd->cmd == req_cmds->cmds[i]) {
					return 0;
				}
			}
			printf("Invalid command: '%s'. Does not meet requirement\n", CMD_NAMES[cmd->cmd]);
		}
	}
}