#ifndef _DBERROR_H_
#define _DBERROR_H_

#include <stdlib.h>

/*
 * Errors
 */

typedef enum {
	ERR_GENERAL,
	ERR_CLIENT_EXIT,
	ERR_INVALID_CMD
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