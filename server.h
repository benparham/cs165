#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdlib.h>
#include "database.h"

#define BACKLOG 				10
#define HOST_LOOKUP_CMD 		"ifconfig | grep -P 'inet (?!127.0.0.1)'"
#define MAX_ARG_LEN				256

typedef struct threadArgs {
	int socketFD;
} threadArgs;

void *listenToClient();
void terminateConnection(int socketFD);
// int cmdUse(int socketFD, FILE *fp);
int requireTable(dbTable *tbl, int socketFD, command *cmd, error *err);

#endif