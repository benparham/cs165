#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "data.h"
#include "error.h"

columnCache *colCache;

// =================================================================
// Column Buffer Manipulation
// =================================================================

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

// =================================================================
// Data Overhead Setup/Cleanup
// =================================================================

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

// =================================================================
// Cache Management
// =================================================================

/* 
 * Assumes that nameCache is locked.
 * Checks nameCache for columnName.
 * Returns index of hit on success, -1 on failure.
 */

int getColIdx(char *columnName) {
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		if (strcmp(colCache->nameCache.names[i], columnName) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * Assumes that nameCache is locked.
 */

columnBuf* fetchColFromDisk(char *columnName, error *err) {

	err->err = ERR_INTERNAL;
	err->message = "fetch from disk not yet implemented";
	return NULL;
}

/*
 * Assumes dataBootstrap has been called
 * Fetches the column buffer with name = columnName
 * On success, locks the column buffer and returns pointer to it
 * On failure returns NULL
 */

columnBuf* fetchCol(char *columnName, error *err) {
	columnBuf *colBuf;

	// Lock the name cache
	pthread_mutex_lock(&(colCache->nameCache.nameLock));

	// Check cache
	int idx = getColIdx(columnName);
	if (idx == -1) {
		colBuf = fetchColFromDisk(columnName, err);		// Fetch from disk
	} else {
		colBuf = colCache->bufCache[idx];				// Fetch from cache
		
		if (colBuf == NULL) {
			err->err = ERR_INTERNAL;
			err->message = "Name cache and buffer cache out of synch";
		}
	}

	// Lock the column Buffer
	if (colBuf != NULL) {
		pthread_mutex_lock(&(colBuf->colLock));
	}

	// Release the name cache lock
	pthread_mutex_unlock(&(colCache->nameCache.nameLock));

	return colBuf;
}








