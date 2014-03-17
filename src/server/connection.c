#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <connection.h>
#include <command.h>
#include <error.h>
#include <table.h>

int connectionCreate(connection **con, threadArgs *tArgs) {
	*con = (connection *) malloc(sizeof(connection));
	if (*con == NULL) {
		return 1;
	}

	(*con)->tArgs = tArgs;
	(*con)->cmd = createCommand();
	(*con)->err = (error *) malloc(sizeof(error));
	(*con)->tbl = (tableInfo *) malloc(sizeof(tableInfo));

	return 0;
}

void connectionDestroy(connection *con) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", con->tArgs->socketFD);
	close(con->tArgs->socketFD);

	free(con->tArgs);
	destroyCommand(con->cmd);
	free(con->err);
	free(con->tbl);

	free(con);
}

int connectionReceiveCommand(connection *con) {//int socketFD, command *cmd, error *err) {
	int socketFD = con->tArgs->socketFD;
	command *cmd = con->cmd;
	error *err = con->err;

	if (cmd->args != NULL) {
		free(cmd->args);
		cmd->args = NULL;
	}

	char buf[BUFSIZE];
	int bytesRecieved;

	printf("Waiting to receive command from client...\n");
	memset(buf, 0, BUFSIZE);

	bytesRecieved = recv(socketFD, buf, BUFSIZE, 0);
	if (bytesRecieved < 1) {
		ERROR(err, E_EXIT);
		return 1;
	}
	// if (bytesRecieved < 1) {
	// 	err->err = ERR_CLIENT_EXIT;
	// 	err->message = "Client has closed connection";
	// 	return 1;
	// }

	return parseCommand(buf, cmd, err);
}

int connectionSendError(connection *con) {
	char *message;
	
	if (handleError(con->err, &message)) {
		return 1;
	}

	// Send message to the user

	return 0;
}