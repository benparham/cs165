#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdlib.h>

#include <error.h>
#include <command.h>

#define BACKLOG 				10
#define HOST_LOOKUP_CMD 		"ifconfig | grep -P 'inet (?!127.0.0.1)'"

typedef struct threadArgs {
	int socketFD;
} threadArgs;

void *listenToClient();
void terminateConnection(int socketFD);
int receiveCommand(int socketFD, command *cmd, error *err);
int bootstrap();
void cleanup();

#endif