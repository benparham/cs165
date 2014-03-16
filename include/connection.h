#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <command.h>
#include <error.h>
#include <table.h>

typedef struct threadArgs {
	int socketFD;
} threadArgs;

typedef struct connection {
	threadArgs *tArgs;
	command *cmd;
	error *err;
	tableInfo *tbl;
} connection;

int initConnection(connection *con, threadArgs *tArgs);

int destroyConnection(connection *con);

#endif