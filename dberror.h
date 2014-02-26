#ifndef _DBERROR_H_
#define _DBERROR_H_

#include <stdlib.h>

/*
 * Errors
 */

typedef enum {
	ERR_GENERAL,			// General error
	ERR_CLIENT_EXIT,		// Client has exited
	ERR_INVALID_CMD,		// Invalid command

	ERR_MLFM_DATA,			// Malformed data, corrupted or incorrect data found
	ERR_SRCH,				// Search failed, something not found
	ERR_INTERNAL,			// Internal error, not users='s fault

	ERR_DUP					// Duplicate, already exiists
} ERR;

// Create new type called 'error'

typedef struct error {
	ERR err;
	char *message;
} error;

// Error functions
int handleReceiveErrors(error *err);
int handleExecuteErrors(error *err);

#endif