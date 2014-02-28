#ifndef _DATA_H_
#define _DATA_H_

#include "global.h"
#include "column.h"
#include "pthread.h"

// TODO: make it so we just constantly check the byte size of the cache,
// rather than have a fixed number of unknown sized columns
#define COL_CACHE_SIZE		4

typedef struct rawData {
	unsigned char *data;
	pthread_mutex_t dataLock;
} rawData;

typedef struct colBuf {
	columnInfo colInfo;
	rawData colData;
} colBuf;

typedef struct colCache {
	colBuf *colCache[COL_CACHE_SIZE];
	pthread_mutex_t cacheLock;
} colCache;

extern colCache *columnCache;

void colCacheInit(colCache *cache);
void colCacheDestroy(colCache *cache);

int dataBootstrap();
void dataCleanup();

// colBuf *getColBuf();
// void releaseColBuf(colBuf *colBuf);


#endif