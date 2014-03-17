#include <stdlib.h>
#include <stdio.h>

#include <error.h>
#include <global.h>

char *errorMessages[] = {
	"No error",
	"User has exited",
	"Not yet implemented",
	"Unsupported column storage type",
	"Out of memory",
	"Column does not exist",
	"Column already exists",
	"Could not open file",
	"Could not read from file",
	"Could not write to file",
	"Could not seek in file",
	"Could not flush file",
	"Column is empty",
	"Out of range",
	"Fetch on column of unequal size",
	"Unknown command",
	"Invalid argument(s)",
	"Table does not exist",
	"Table already exists",
	"No table in use",
	"Could not make directory",
	"Could not open directory",
	"Could not remove directory",
	"Could not remove file",
	"Could not create new var map node"
};

void recordError(error *err, int errno) {
	err->errno = errno;
}

void recordErrorDebug(error *err, int errno, const char *fileName, const char *funcName, int lineNum) {
	recordError(err, errno);
	err->fileName = fileName;
	err->funcName = funcName;
	err->lineNum = lineNum;
}

int handleError(error *err, char **message) {
	int result;

	*message = errorMessages[err->errno];

	switch (err->errno) {
		case E_EXIT:
			result = 1;
			break;
		default:
			result = 0;
			break;
	}

	printf("Error: %s\n", *message);

	return result;
}

// // Returns 1 if server needs to shut down client's thread, 0 otherwises
// int handleReceiveErrors(error *err) {
// 	int result = 0;

// 	switch (err->err) {
// 		case ERR_CLIENT_EXIT:
// 			result = 1;
// 			break;
// 		case ERR_GENERAL:
// 		case ERR_INVALID_CMD:
// 			break;
// 		default:
// 			err->message = "Unknown error";
// 			break;
// 	}

// 	printf("%s\n", err->message);
// 	return result;
// }

// // Returns 1 if server needs to shut down client's thread, 0 otherwise
// int handleExecuteErrors(error *err) {
// 	int result = 0;

// 	switch (err->err) {
// 		case ERR_CLIENT_EXIT:
// 			result = 1;
// 			break;
// 		default:
// 			// if (err->message == NULL || err->message == " ");
// 			// err->message = "Unknown error";
// 			break;
// 	}

// 	printf("%s\n", err->message);
// 	return result;
// }