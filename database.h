#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include "dberror.h"

#define BUFSIZE				1024


/*
 * Tables
 */

typedef struct table {
	char *name;
	int numRows;
	char **columns;
} dbTable;

/*
 * Commands
 */

// Declaration of global array of command strings matched to enum CMD
extern const char *CMD_NAMES[]; 

// Enumerate possible commands
typedef enum {
	CMD_USE,
	CMD_SELECT,
	CMD_FETCH,
	CMD_CREATE,
	CMD_LOAD,
	CMD_INSERT,
	CMD_EXIT
} CMD;

// Create new type called 'command'
typedef struct command {
	CMD cmd;
	char *args;
} command;

// Command functions

int parseCommand(char *buf, command *cmd, error *err);

int receiveCommand(int socketFD, command *cmd, error *err);

int requireCommand(CMD req_cmd, int socketFD, command *cmd, error *err);

#endif