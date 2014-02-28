#ifndef _DATA_H_
#define _DATA_H_

#include "global.h"
#include "column.h"
#include "pthread.h"

// TODO: make it so we just constantly check the byte size of the cache,
// rather than have a fixed number of unknown sized columns
#define COL_CACHE_SIZE		4

typedef struct columnBuf {
	columnInfo colInfo;
	unsigned char *data;
	pthread_mutex_t colLock;
} columnBuf;

typedef struct columnCache {
	columnBuf *bufCache[COL_CACHE_SIZE];
	struct nameCache {
		char *names[COL_CACHE_SIZE];
		pthread_mutex_t nameLock;
	} nameCache;
} columnCache;

extern columnCache *colCache;

int dataBootstrap();
void dataCleanup();


#endif