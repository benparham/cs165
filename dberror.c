#include <stdlib.h>
#include <stdio.h>
#include "dberror.h"

// Returns 1 if server needs to shut down client's thread, 0 otherwise
int handleReceiveErrors(error *err) {
	int result = 0;

	switch (err->err) {
		case ERR_CLIENT_EXIT:
			result = 1;
			break;
		case ERR_GENERAL:
		case ERR_INVALID_CMD:
			break;
		default:
			err->message = "Unknown error";
			break;
	}

	printf("%s\n", err->message);
	return result;
}

// Returns 1 if server needs to shut down client's thread, 0 otherwise
int handleExecuteErrors(error *err) {
	int result = 0;

	switch (err->err) {
		default:
			err->message = "Unknown error";
			break;
	}

	printf("%s\n", err->message);
	return result;
}