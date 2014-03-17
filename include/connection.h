#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <command.h>
#include <error.h>
#include <response.h>
#include <table.h>

typedef struct threadArgs {
	int socketFD;
} threadArgs;

typedef struct connection {
	threadArgs *tArgs;
	command *cmd;
	response *res;
	error *err;
	tableInfo *tbl;
} connection;

int connectionCreate(connection **con, threadArgs *tArgs);
void connectionDestroy(connection *con);

int connectionReceiveCommand(connection *con);
// int receiveData(int socketFD, void *data, error *err);

int connectionSendError(connection *con);
int connectionSendResponse(connection *con);

#endif