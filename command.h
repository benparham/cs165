#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <stdlib.h>

#include "dberror.h"

#define BUFSIZE				1024

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

// typedef struct CMD_LIST {
// 	CMD *cmds;
// 	int length;
// } CMD_LIST;

// Create new type called 'command'
typedef struct command {
	CMD cmd;
	char *args;
} command;

// Command functions

command* createCommand();
void destroyCommand(command *cmd);

int parseCommand(char *buf, command *cmd, error *err);

int receiveCommand(int socketFD, command *cmd, error *err);

// int requireCommand(CMD_LIST *req_cmds, int socketFD, command *cmd, error *err);

#endif