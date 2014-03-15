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
int setArgsString(command *cmd, char *args, char *varName) {
	(void) varName;

	cmd->args = (char *) malloc(sizeof(char) * strlen(args));
	strcpy(cmd->args, args);

	return 0;
}

int setArgsNull(command *cmd, char *args, char *varName) {
	(void) args;
	(void) varName;

	cmd->args = NULL;

	return 0;
}

int setArgsColArgs(command *cmd, char *args, char *varName) {
	(void) varName;

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
	// ccArgs->dataType = COL_INT;

	cmd->args = ccArgs;

	return 0;
}

int setArgsInsertArgs(command *cmd, char *args, char *varName) {
	(void) varName;

	char *columnName = strtok(args, ",");
	char *value = strtok(NULL, "\n");

	if (columnName == NULL || value == NULL) {
		return 1;
	}

	insertArgs *insArgs = (insertArgs *) malloc(sizeof(insertArgs));
	strcpy(insArgs->columnName, columnName);
	insArgs->value = atoi(value);
	// strcpy(insArgs->value, value);

	cmd->args = insArgs;

	return 0;	
}

int setArgsSelectArgs(command *cmd, char *args, char *varName) {
	if (varName == NULL) {
		return 1;
	}

	char *columnName = strtok(args, ",");
	char *low = strtok(NULL, ",");
	char *high = strtok(NULL, "\n");

	if (columnName == NULL) {
		return 1;
	}

	selectArgs *selArgs  = (selectArgs *) malloc(sizeof(selectArgs));
	if (selArgs == NULL) {
		return 1;
	}

	selArgs->hasCondition = false;
	selArgs->isRange = false;

	strcpy(selArgs->columnName, columnName);
	strcpy(selArgs->varName, varName);

	if (low != NULL) {
		selArgs->hasCondition = true;
		selArgs->low = atoi(low);

		if (high != NULL) {
			selArgs->isRange = true;
			selArgs->high = atoi(high);
		}
	}

	cmd->args = selArgs;

	printf("Select arguments:\n");
	printf("Column name: %s\n", ((selectArgs *) cmd->args)->columnName);
	printf("Variable name: %s\n", ((selectArgs *) cmd->args)->varName);
	printf("Has condition: %s\n", ((selectArgs *) cmd->args)->hasCondition ? "true" : "false");
	printf("Is range: %s\n", ((selectArgs *) cmd->args)->isRange ? "true" : "false");
	printf("Low: %d\n", ((selectArgs *) cmd->args)->low);
	printf("High: %d\n", ((selectArgs *) cmd->args)->high);

	return 0;
}

struct cmdParseItem {
	char *cmdString;
	char *cmdTerm;
	CMD cmdEnum;
	int (* setArgs)(command *cmd, char *args, char *varName);
};

const struct cmdParseItem cmdParseMap[] = {
	{"use ", "\n", CMD_USE, &setArgsString},
	{"create table ", "\n", CMD_CREATE_TABLE, &setArgsString},
	{"remove table ", "\n", CMD_REMOVE_TABLE, &setArgsString},
	{"create(", ")", CMD_CREATE, &setArgsColArgs},
	{"select(", ")", CMD_SELECT, &setArgsSelectArgs},
	{"insert(", ")", CMD_INSERT, &setArgsInsertArgs},
	{"fetch(", ")", CMD_FETCH, &setArgsString},
	{"load(", ")", CMD_LOAD, &setArgsString},
	{"exit", "\n", CMD_EXIT, &setArgsNull},
	{NULL, NULL, 0, NULL}
};

int parseCommand(char *buf, command *cmd, error *err) {

	// Check for user variable
	char *varName = strtok(buf, "=");
	char *newBuf = strtok(NULL, "\0");
	
	// If there is no user variable
	if (newBuf == NULL) {
		newBuf = varName;
		varName = NULL;
	}

	int i;
	char *cmdString;
	for (i = 0; (cmdString = cmdParseMap[i].cmdString) != NULL; i++) {

		int cmdLen = strlen(cmdString);
		if (strncmp(newBuf, cmdString, cmdLen) == 0) {
			cmd->cmd = cmdParseMap[i].cmdEnum;

			char *cmdTerm = cmdParseMap[i].cmdTerm;
			if (cmdParseMap[i].setArgs(cmd, strtok(&newBuf[cmdLen], cmdTerm), varName)) {
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