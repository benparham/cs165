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
	CMD_PRINT_VAR,				// Print an intermediate variable in varmap

	CMD_SELECT,					
	CMD_FETCH,
	
	CMD_CREATE,					// Create a column
	CMD_REMOVE,					// Remove a column
	CMD_LOAD,
	CMD_INSERT,
	CMD_PRINT,					// Print a column

	// Aggregation
	CMD_MIN,
	CMD_MAX,
	CMD_SUM,
	CMD_AVG,
	CMD_CNT,

	// Math
	CMD_ADD,
	CMD_SUB,
	CMD_MUL,
	CMD_DIV,
	
	CMD_EXIT					// End session
} CMD;

// Create new type called 'command'
typedef struct command {
	CMD cmd;
	void *args;
} command;

typedef struct createColArgs {
	char columnName[BUFSIZE];
	COL_STORAGE_TYPE storageType;
} createColArgs;

typedef struct insertArgs {
	char columnName[BUFSIZE];
	int value;
} insertArgs;

typedef struct selectArgs {
	char columnName[BUFSIZE];		// Name of the column from which to select
	char varName[BUFSIZE];			// Name of the variable to store the intermediate result in
	
	bool hasCondition;				// False if we are selecting the whole column, true otherwise
	bool isRange;					// True if a range of values is given, else low == high == single value

	int low;
	int high;
} selectArgs;

typedef struct fetchArgs {
	char columnName[BUFSIZE];
	char oldVarName[BUFSIZE];
	char newVarName[BUFSIZE];
} fetchArgs;

typedef struct loadArgs {
	int numColumns;
	char **columnNames;
} loadArgs;

typedef struct mathArgs {
	char var1[BUFSIZE];
	char var2[BUFSIZE];
} mathArgs;

command* createCommand();
void destroyCommandArgs(command *cmd);
void destroyCommand(command *cmd);

createColArgs* createCCA();
void destroyCCA(createColArgs *args);

int parseCommand(char *buf, command *cmd, error *err);

#endif