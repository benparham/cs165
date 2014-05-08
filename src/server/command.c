#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <command.h>
#include <error.h>


// Definition of global array of command strings matched to enum CMD
const char *CMD_NAMES[] = {
	"use",
	"create table",
	"remove table",
	"print var",
	"select",
	"fetch",
	"create",
	"remove",
	"load",
	"insert",
	"print",
	"minimum",
	"maximum",
	"sum",
	"average",
	"count",
	"add",
	"subtract",
	"multiply",
	"divide",
	"loop join",
	"exit"
};


/*
 * Parsing Commands
 */

// TODO: Change this so that there's one single setArgs function. Then the map doesn't need a function
//		 pointer. We switch on cmd->cmd in the new function and do the appropriate thing

// TODO: check that we are freeing these args eventually!!!! MEMORY LEAK ahhhhh!!!!!
int setArgsString(command *cmd, char *args, char *ignore) {
	(void) ignore;

	cmd->args = (char *) malloc(sizeof(char) * strlen(args));
	strcpy(cmd->args, args);

	return 0;
}

int setArgsNull(command *cmd, char *args, char *ignore) {
	(void) args;
	(void) ignore;

	cmd->args = NULL;

	return 0;
}

int setArgsColArgs(command *cmd, char *args, char *ignore) {
	(void) ignore;

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

int setArgsInsertArgs(command *cmd, char *args, char *ignore) {
	(void) ignore;

	char *columnName = strtok(args, ",");
	char *value = strtok(NULL, "\n");

	if (columnName == NULL || value == NULL) {
		return 1;
	}

	insertArgs *insArgs = (insertArgs *) malloc(sizeof(insertArgs));
	strcpy(insArgs->columnName, columnName);
	insArgs->value = atoi(value);

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

	return 0;
}

int setArgsFetchArgs(command *cmd, char *args, char *newVarName) {

	char *columnName = strtok(args, ",");
	char *oldVarName = strtok(NULL, "\n");

	if (columnName == NULL || oldVarName == NULL) {
		return 1;
	}

	fetchArgs *fetArgs = (fetchArgs *) malloc(sizeof(fetchArgs));
	if (fetArgs == NULL) {
		return 1;
	}

	strcpy(fetArgs->columnName, columnName);
	strcpy(fetArgs->oldVarName, oldVarName);

	if (newVarName != NULL) {
		strcpy(fetArgs->newVarName, newVarName);
	} else {
		fetArgs->newVarName[0] = '\0';
	}

	cmd->args = fetArgs;

	return 0;
}

int setArgsLoadArgs(command *cmd, char *args, char *ignore) {
	(void) ignore;

	loadArgs *ldArgs = (loadArgs *) malloc(sizeof(loadArgs));
	if (ldArgs == NULL) {
		return 1;
	}
	ldArgs->columnNames = NULL;

	char *curColName = strtok(args, ",\n");

	int idx = 0;
	while(curColName != NULL) {
		char **temp = (char **) realloc(ldArgs->columnNames, (idx + 1) * sizeof(char *));
		if (temp == NULL) {
			goto cleanupArgs;
		}
		ldArgs->columnNames = temp;

		ldArgs->columnNames[idx] = (char *) malloc((strlen(curColName) + 1) * sizeof(char));
		if (ldArgs->columnNames[idx] == NULL) {
			goto cleanupArgs;
		}
		
		strcpy(ldArgs->columnNames[idx], curColName);

		curColName = strtok(NULL, ",\n");
		idx += 1;
	}

	ldArgs->numColumns = idx;

	cmd->args = ldArgs;

	return 0;

cleanupArgs:
	for (int i = 0; i < idx; i++) {
		free(ldArgs->columnNames[i]);
	}

	if (ldArgs->columnNames != NULL) {
		free(ldArgs->columnNames);
	}

	free(ldArgs);

	return 1;
}

int setArgsMathArgs(command *cmd, char *args, char *ignore) {

	char *var1 = strtok(args, ",");
	char *var2 = strtok(NULL, "\n");

	if (var1 == NULL || var2 == NULL) {
		return 1;
	}

	mathArgs *mthArgs = (mathArgs *) malloc(sizeof(mathArgs));
	if (mthArgs == NULL) {
		return 1;
	}

	strcpy(mthArgs->var1, var1);
	strcpy(mthArgs->var2, var2);

	cmd->args = mthArgs;

	return 0;
}

int setArgsJoinArgs(command *cmd, char *args, char *newVarNames) {
	char *oldVar1 = strtok(args, ",");
	char *oldVar2 = strtok(NULL, "\n");

	char *newVar1 = strtok(newVarNames, ",");
	char *newVar2 = strtok(NULL, "\n");

	if (oldVar1 == NULL || oldVar2 == NULL ||
		newVar1 == NULL || newVar2 == NULL) {
		return 1;
	}

	joinArgs *jnArgs = (joinArgs *) malloc(sizeof(joinArgs));
	if (jnArgs == NULL) {
		return 1;
	}

	strcpy(jnArgs->oldVar1, oldVar1);
	strcpy(jnArgs->oldVar2, oldVar2);

	strcpy(jnArgs->newVar1, newVar1);
	strcpy(jnArgs->newVar2, newVar2);

	cmd->args = jnArgs;

	return 0;
}

void destroyLoadArgs(loadArgs *ldArgs) {
	int length = sizeof(ldArgs->columnNames) / sizeof(ldArgs->columnNames[0]);

	for (int i = 0; i < length; i++) {
		free(ldArgs->columnNames[i]);
	}

	free(ldArgs->columnNames);
	free(ldArgs);
}

command* createCommand() {
	command* cmd = (command *) malloc(sizeof(command));
	cmd->args = NULL;
	return cmd;
}

void destroyCommandArgs(command *cmd) {
	if (cmd->args == NULL) {
		return;
	}

	switch(cmd->cmd) {
		case CMD_LOAD:
			destroyLoadArgs(cmd->args);
			break;
		default:
			free(cmd->args);
	}

	cmd->args = NULL;
}

void destroyCommand(command *cmd) {
	// if (cmd->args != NULL) {
	// 	free(cmd->args);
	// }
	destroyCommandArgs(cmd);
	free(cmd);
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
	{"print var ", "\n", CMD_PRINT_VAR, &setArgsString},
	{"create(", ")", CMD_CREATE, &setArgsColArgs},
	{"remove(", ")", CMD_REMOVE, &setArgsString},
	{"select(", ")", CMD_SELECT, &setArgsSelectArgs},
	{"insert(", ")", CMD_INSERT, &setArgsInsertArgs},
	{"fetch(", ")", CMD_FETCH, &setArgsFetchArgs},
	{"load(", ")", CMD_LOAD, &setArgsLoadArgs},
	{"print(", ")", CMD_PRINT, &setArgsString},
	{"min(", ")", CMD_MIN, &setArgsString},
	{"max(", ")", CMD_MAX, &setArgsString},
	{"sum(", ")", CMD_SUM, &setArgsString},
	{"avg(", ")", CMD_AVG, &setArgsString},
	{"count(", ")", CMD_CNT, &setArgsString},
	{"add(", ")", CMD_ADD, &setArgsMathArgs},
	{"sub(", ")", CMD_SUB, &setArgsMathArgs},
	{"mul(", ")", CMD_MUL, &setArgsMathArgs},
	{"div(", ")", CMD_DIV, &setArgsMathArgs},
	{"loopjoin(", ")", CMD_LOOPJOIN, &setArgsJoinArgs},
	{"exit", "\n", CMD_EXIT, &setArgsNull},
	{NULL, NULL, 0, NULL}
};

int parseCommand(char *buf, command *cmd, error *err) {

	if (buf == NULL || strcmp(buf, "") == 0) {
		ERROR(err, E_BADARG);
		return 1;
	}

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
				ERROR(err, E_BADARG);
				return 1;
			}

			return 0;
		}
	}

	ERROR(err, E_UNKCMD);
	printf("Unknown command: %s\n", buf);
	return 1;
}