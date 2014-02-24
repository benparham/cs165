#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#define BUFSIZE				1024

/*
 * Errors
 */

typedef enum {
	ERR_GENERAL,
	ERR_CLIENT_EXIT,
	ERR_INVALID_CMD
} ERR;

// Create new type called 'error'

typedef struct error {
	ERR err;
	char *message;
} error;


/*
 * Commands
 */

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