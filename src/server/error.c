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
	"Could not create new var map node",
	"Message error",
	"Internal error",
	"Variable does not exist",
	"Serialization error",
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
	
	#ifdef DEBUG
	printf("Error: %s\n", *message);
	printf("File '%s', Function '%s', Line '%d'\n", err->fileName, err->funcName, err->lineNum);
	#endif

	// Wipe error
	err->errno = E_INTERN;
	err->fileName = "NA";
	err->funcName = "NA";
	err->lineNum = -1;

	return result;
}