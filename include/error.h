#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdlib.h>

#include <global.h>

#ifdef DEBUG
#define ERROR(ERR, ERRNO)	recordErrorDebug(ERR, ERRNO, __FILE__, __FUNCTION__, __LINE__)
#else
#define ERROR(ERR, ERRNO)	recordError(ERR, ERRNO)
#endif

#define E_EXIT 				1		// User has exited
#define E_UNIMP 			2		// Procedure is unimplemented
#define E_COLST				3		// Unsupported column storage type
#define E_NOMEM				4		// Not enough memory
#define E_NOCOL				5		// Column does not exist
#define E_DUPCOL			6		// Column already exists
#define E_FOP				7		// Failed to open file
#define E_FRD				8		// Failed to read from file
#define E_FWR				9		// Failed to write to file
#define E_FSK				10		// Failed to seek in file
#define E_FFL				11		// Failed to flush file
#define E_COLEMT			12		// Column empty
#define E_OUTRNG			13		// Out of range
#define E_BADFTC			14		// Fetch on column with unequal size

#define E_UNKCMD			15		// Unknown command
#define E_BADARG			16		// Invalid argument(s)

#define E_NOTBL				17		// Table does not exist
#define E_DUPTBL			18		// Table already exists
#define E_USETBL			19		// No table in use

#define E_MKDIR				20		// Failed to make directory
#define E_DOP				21		// Failed to open directory
#define E_DRM				22		// Failed to remove directory

#define E_FRM				23		// Failed to remove file

#define E_VARND				24		// Failed to create var map node

#define E_MSG				25		// Message error

#define E_INTERN 			26		// Internal error

#define E_NOVAR				27		// Variable does not exist

#define E_SRL				28		// Serialization error


/*
 * Errors
 */

// typedef enum {
// 	ERR_GENERAL,			// General error
// 	ERR_CLIENT_EXIT,		// Client has exited
// 	ERR_INVALID_CMD,		// Invalid command

// 	ERR_MLFM_DATA,			// Malformed data, corrupted or incorrect data found
// 	ERR_SRCH,				// Search failed, something not found
// 	ERR_INTERNAL,			// Internal error, not users='s fault

// 	ERR_DUP,				// Duplicate, already exiists

// 	ERR_MEM,				// Memory err, usually indicates not enough memory free for operation

// 	ERR_UNIMP				// Procedure is not yet implemented
// } ERR;

typedef struct error {
	// ERR err;
	// char *message;
	int errno;
	#ifdef DEBUG
	const char *fileName;
	const char *funcName;
	int lineNum;
	#endif
} error;

void recordError(error *err, int errno);
void recordErrorDebug(error *err, int errno, const char *fileName, const char *funcName, int lineNum);

int handleError(error *err, char **message);

// // Error functions
// int handleReceiveErrors(error *err);
// int handleExecuteErrors(error *err);

#endif