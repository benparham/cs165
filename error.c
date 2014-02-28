#include <stdlib.h>
#include <stdio.h>

#include "error.h"

// Returns 1 if server needs to shut down client's thread, 0 otherwises
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
		case ERR_CLIENT_EXIT:
			result = 1;
			break;
		default:
			// if (err->message == NULL || err->message == " ");
			// err->message = "Unknown error";
			break;
	}

	printf("%s\n", err->message);
	return result;
}