#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "data.h"

columnCache *colCache;

columnBuf* colBufCreate() {
	columnBuf *colBuf = (columnBuf *) malloc(sizeof(columnBuf));
	pthread_mutex_init(&(colBuf->colLock), NULL);

	return colBuf;
}

void colBufSet(columnBuf *colBuf, columnInfo colInfo, unsigned char *data) {
	colBuf->colInfo = colInfo;
	colBuf->data = data;
}

void colBufDestroy(columnBuf *colBuf) {
	if (colBuf == NULL) {
		return;
	}

	// Free data if not null
	if (colBuf->data != NULL) {
		free(colBuf->data);
	}

	// Destroy the column buffer lock
	pthread_mutex_destroy(&(colBuf->colLock));

	// Free the whole column buffer
	free(colBuf);
}

int dataBootstrap() {
	// Allocate the whole column cache
	colCache = (columnCache *) malloc(sizeof(columnCache));

	// Ensure that all column buffer pointers point to null
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		colCache->bufCache[i] = NULL;
	}

	// Create the name cache lock
	pthread_mutex_init(&(colCache->nameCache.nameLock), NULL);

	return 0;
}

void dataCleanup() {
	// Iterate over bufCache and ensure all colBufs are destroyed
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		if (colCache->bufCache[i] != NULL) {
			colBufDestroy(colCache->bufCache[i]);
		}
	}

	// Destroy the name cache lock
	pthread_mutex_destroy(&(colCache->nameCache.nameLock));

	// Free the whole column cache
	free(colCache);
}

