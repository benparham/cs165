#include <sys/socket.h>

#include <connection.h>
#include <command.h>
#include <error.h>
#include <table.h>

int createConnection(connection *con, threadArgs *tArgs) {
	con = (connection *con) malloc(sizeof(connction));
	if (con == NULL) {
		return 1;
	}

	con->tArgs = tArgs;
	con->cmd = createCommand();
	con->err = (error *) malloc(sizeof(error));
	con->tbl = (tableInfo *) malloc(sizeof(tableInfo));

	return 0;
}

void destroyConnection(connection *con) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", con->tArgs->socketFD);
	close(con->tArgs->socketFD);

	free(con->tArgs);
	destoryCommand(con->cmd);
	free(con->err);
	free(con->tbl);

	free(con);
}