#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <assert.h>

#define DEBUG
#define ASSERTS

#ifdef ASSERTS
	#define MY_ASSERT(x)	assert(x)
#else
	#define MY_ASSERT(x)
#endif

#define BUFSIZE				1024			// Size of buffer used for string manipulation

#define NAME_SIZE			16				// Maximum size of table/column names
#define MAX_COLS			16				// Maximum number of columns per table

typedef enum {
	false = 0,
	true = 1
} bool;

#endif