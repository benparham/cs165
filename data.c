#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "data.h"

colCache *columnCache;

void colCacheInit(colCache *cache) {
	cache = (colCache *) malloc(sizeof(colCache));
	pthread_mutex_init(&(cache->cacheLock), NULL);
}

void colCacheDestroy(colCache *cache) {
	pthread_mutex_destroy(&(cache->cacheLock));
	free(cache);
}

int dataBootstrap() {
	colCacheInit(columnCache);
	return 0;
}

void dataCleanup() {
	colCacheDestroy(columnCache);
}

