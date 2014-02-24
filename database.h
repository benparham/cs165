#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#define E_MSG_SIZE			256

// Commands
#define CMD_USE				'use'

// Error Codes
#define ECODE_GENERAL		0

typedef struct error {
	int code;
	char message[E_MSG_SIZE];
} ERROR;


/*
 * Error functions
 */
ERROR* createError();

void setError(ERROR *err, int code, char *msg);

void destroyError(ERROR *err);


typedef enum {

} CMD;


#endif