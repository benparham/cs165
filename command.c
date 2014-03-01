#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "command.h"
#include "error.h"


// Definition of global array of command strings matched to enum CMD
const char *CMD_NAMES[] = {
	"use",
	"create table",
	"remove table",
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

void destroyCommand(command *cmd) {
	if (cmd->args != NULL) {
		free(cmd->args);
	}

	free(cmd);
}


/*
 * Parsing Commands
 */

// TODO: Change this so that there's one single setArgs function. Then the map doesn't need a function
//		 pointer. We switch on cmd->cmd in the new function and do the appropriate thing

// TODO: check that we are freeing these args eventually!!!! MEMORY LEAK ahhhhh!!!!!
int setArgsString(command *cmd, char *args) {
	cmd->args = (char *) malloc(sizeof(char) * strlen(args));
	strcpy(cmd->args, args);

	return 0;
}

int setArgsNull(command *cmd, char *args) {

	(void) args;

	cmd->args = NULL;

	return 0;
}

int setArgsColArgs(command *cmd, char *args) {
	char *columnName = strtok(args, ",");
	char *columnStorageType = strtok(NULL, "\n");
	
	if (columnName == NULL || columnStorageType == NULL) {
		return 1;
	}

	COL_STORAGE_TYPE storageType;
	if (strToColStorage(columnStorageType, &storageType)) {
		return 1;
	}

	createColArgs *ccArgs = (createColArgs *) malloc(sizeof(createColArgs));
	strcpy(ccArgs->columnName, columnName);
	ccArgs->storageType = storageType;
	ccArgs->dataType = COL_INT;

	cmd->args = ccArgs;

	return 0;
}

// int setArgsInsertArgs(command *cmd, char *args) {
// 	char *columnName = strtok(args, ",");
// 	char *value = strtok(NULL, "\n");

// 	if (columnName == NULL || value == NULL) {
// 		return 1;
// 	}


// }

struct cmdParseItem {
	char *cmdString;
	char *cmdTerm;
	CMD cmdEnum;
	int (* setArgs)(command *, char *);
};

const struct cmdParseItem cmdParseMap[] = {
	{"use ", "\n", CMD_USE, &setArgsString},
	{"create table ", "\n", CMD_CREATE_TABLE, &setArgsString},
	{"remove table ", "\n", CMD_REMOVE_TABLE, &setArgsString},
	{"create(", ")", CMD_CREATE, &setArgsColArgs},
	{"select(", ")", CMD_SELECT, &setArgsString},
	// {"insert(", ")", CMD_INSERT, &setArgsInsertArgs},
	{"fetch(", ")", CMD_FETCH, &setArgsString},
	{"load(", ")", CMD_LOAD, &setArgsString},
	{"exit", "\n", CMD_EXIT, &setArgsNull},
	{NULL, NULL, 0, NULL}
};

int parseCommand(char *buf, command *cmd, error *err) {
	
	
	int i;
	char *cmdString;
	for (i = 0; (cmdString = cmdParseMap[i].cmdString) != NULL; i++) {

		int cmdLen = strlen(cmdString);
		if (strncmp(buf, cmdString, cmdLen) == 0) {
			cmd->cmd = cmdParseMap[i].cmdEnum;

			char *cmdTerm = cmdParseMap[i].cmdTerm;
			if (cmdParseMap[i].setArgs(cmd, strtok(&buf[cmdLen], cmdTerm))) {
				err->err = ERR_INVALID_CMD;
				err->message = "Arguments are invalid";
				return 1;
			}

			return 0;
		}
	}

	err->err = ERR_INVALID_CMD;
	err->message = "Unknown command";
	return 1;
}