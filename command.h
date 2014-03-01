#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <stdlib.h>

#include "error.h"
#include "global.h"
#include "column.h"

/*
 * Commands
 */

// Declaration of global array of command strings matched to enum CMD
extern const char *CMD_NAMES[];

// Enumerate possible commands
typedef enum {
	CMD_USE,					// Use a table
	CMD_CREATE_TABLE,			// Create a table
	CMD_REMOVE_TABLE,			// Remove a table

	CMD_SELECT,					
	CMD_FETCH,
	
	CMD_CREATE,					// Create a column
	CMD_LOAD,
	CMD_INSERT,
	
	CMD_EXIT					// End session
} CMD;

// Create new type called 'command'
typedef struct command {
	CMD cmd;
	void *args;
} command;

typedef struct createColArgs {
	char columnName[BUFSIZE];
	COL_DATA_TYPE dataType;
	COL_STORAGE_TYPE storageType;
} createColArgs;

// typedef struct insertArgs {
// 	char *columnName;
// 	char *value;
// } insertArgs;

command* createCommand();
void destroyCommand(command *cmd);

createColArgs* createCCA();
void destroyCCA(createColArgs *args);

int parseCommand(char *buf, command *cmd, error *err);

#endif