#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <connection.h>
#include <command.h>
#include <error.h>
#include <response.h>
#include <table.h>
#include <message.h>

int connectionCreate(connection **con, threadArgs *tArgs) {
	*con = (connection *) malloc(sizeof(connection));
	if (*con == NULL) {
		return 1;
	}

	(*con)->tArgs = tArgs;
	(*con)->cmd = createCommand();
	(*con)->err = (error *) malloc(sizeof(error));
	if (responseCreate(&((*con)->res))) {
		// TODO: cleanup other stuff here
		return 1;
	}
	(*con)->tbl = (tableInfo *) malloc(sizeof(tableInfo));

	(*con)->dataBytes = 0;
	(*con)->data = NULL;

	return 0;
}

void connectionDestroy(connection *con) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", con->tArgs->socketFD);
	close(con->tArgs->socketFD);

	free(con->tArgs);
	destroyCommand(con->cmd);
	free(con->err);
	responseDestroy(con->res);
	free(con->tbl);

	if (con->data != NULL) {
		free(con->data);
	}

	free(con);
}

int connectionReceiveCommand(connection *con) {
	int socketFD = con->tArgs->socketFD;
	command *cmd = con->cmd;
	error *err = con->err;

	// if (cmd->args != NULL) {
	// 	free(cmd->args);
	// 	cmd->args = NULL;
	// }
	destroyCommandArgs(cmd);

	printf("\n>====== Waiting to receive command from client...\n");

	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);

	int term;
	if (messageReceive(socketFD, buf, &(con->dataBytes), &(con->data), &term)) {
		if (term) {
			ERROR(err, E_EXIT);
		} else {
			ERROR(err, E_MSG);
		}

		return 1;
	}

	return parseCommand(buf, cmd, err);
}

int connectionSendError(connection *con) {
	char *message;
	
	if (handleError(con->err, &message)) {
		return 1;
	}

	// Ignore errors in sending message
	messageSend(con->tArgs->socketFD, message);

	return 0;
}

int connectionSendResponse(connection *con) {
	// char *message;
	// int dataBytes;
	// void *data;

	// if (handleResponse(con->res, &message, &dataBytes, &data)) {
	// 	return 1;
	// }
	int socketFD = con->tArgs->socketFD;
	response *res = con->res;

	if (res->dataBytes > 0) {
		dataSend(socketFD, res->message, res->dataBytes, res->data);
	} else {
		// Ignore errors in sending message
		messageSend(socketFD, res->message);
	}

	responseWipe(res);

	return 0;
}