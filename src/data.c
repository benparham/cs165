#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <data.h>
#include <error.h>
#include <filesys.h>
#include <table.h>

columnCache *colCache;

// =================================================================
// Column Buffer Manipulation
// =================================================================

columnBuf* colBufCreate() {
	columnBuf *colBuf = (columnBuf *) malloc(sizeof(columnBuf));
	pthread_mutex_init(&(colBuf->colLock), NULL);

	return colBuf;
}

// void colBufSet(columnBuf *colBuf, columnInfo colInfo, unsigned char *data) {
// 	if (colBuf->data != NULL) {
// 		free(colBuf->data);
// 	}

// 	memcpy(&(colBuf->colInfo), &(colInfo), sizeof(columnInfo));
// 	// colBuf->colInfo = colInfo;
// 	colBuf->data = data;
// }

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
	colBuf = NULL;
}

// =================================================================
// Data Overhead Setup/Cleanup
// =================================================================

int dataBootstrap() {
	// Allocate the whole column cache
	colCache = (columnCache *) malloc(sizeof(columnCache));

	// Ensure that all cache pointers point to null
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		colCache->nameCache.names[i] = NULL;
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
		if (colCache->nameCache.names[i] != NULL &&
			strcmp(colCache->nameCache.names[i], columnName) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * Assumes that nameCache is locked.
 * Tries to find a null entry in the name cache.
 * Returns index of free column on success, returns -1 if nothing is free.
 */
int getFreeColIdx() {
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		if (colCache->nameCache.names[i] == NULL) {
			return i;
		} 
	}

	return -1;
}


/*
 * Assumes that nameCache is locked.
 * Tried to find an unlocked columnBuf in bufCache.
 * REturns index of unlocked columnBuf on success, returns -1 if nothing is unlocked.
 */
int getUnusedColIdx() {
	int i;
	for (i = 0; i < COL_CACHE_SIZE; i++) {
		columnBuf *colBuf = colCache->bufCache[i];
		if (pthread_mutex_trylock(&(colBuf->colLock)) != 0) {
			return i;
		}
	}

	return -1;
}


/*
 * Assumes that nameCache is locked.
 */

columnBuf* fetchColFromDisk(tableInfo *tbl, char *columnName, error *err) {

	// Check that file exists
	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	if (!fileExists(pathToColumn)) {
		err->err = ERR_SRCH;
		err->message = "Cannot fetch column. Does not exist";
		return NULL;
	}

	columnBuf *colBuf;

	// Get free/unused column buffer idx
	int idx;
	if ((idx = getFreeColIdx()) == -1) {
		if ((idx = getUnusedColIdx()) == -1) {
			err->err = ERR_MEM;
			err->message = "No free column buffers";
			return NULL;
		} else {
			printf("No NULL slots in cache, replacing unused column at index %d\n", idx);
			
			// Get the unused column buffer from the cache and free its data
			colBuf = colCache->bufCache[idx];
			if (colBuf->data != NULL) {
				free(colBuf->data);
			}
		}
	} else {
		printf("Found a NULL slot in cache at index %d\n", idx);

		// Create a new column buffer
		colBuf = colBufCreate();
		pthread_mutex_lock(&(colBuf->colLock));

		// Put it in the cache
		colCache->bufCache[idx] = colBuf;
	}

	// Open column file for reading
	FILE *fp = fopen(pathToColumn, "rb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file for column";
		goto cleanupCache;
	}

	// Read the column info from the beginning
	if (fread(&(colBuf->colInfo), sizeof(columnInfo), 1, fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read column info from file";
		goto cleanupFile;
	}

	// Read in the column data using column info for size
	int sizeBytes = colBuf->colInfo.sizeBytes;
	if (fread(colBuf->data, 1, sizeBytes, fp) < sizeBytes) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read column data from file";
		goto cleanupFile;
	}

	// Update the cache
	colCache->nameCache.names[idx] = colBuf->colInfo.name;

	// Cleanup
	fclose(fp);

	return colBuf;

cleanupFile:
	fclose(fp);
cleanupCache:
	colCache->nameCache.names[idx] = NULL;
	colBufDestroy(colCache->bufCache[idx]);
// exit:
	return NULL;
}

/*
 * Assumes dataBootstrap has been called
 * Fetches the column buffer with name = columnName
 * On success, locks the column buffer and returns pointer to it
 * On failure returns NULL
 */

columnBuf* fetchCol(tableInfo *tbl, char *columnName, error *err) {
	printf("Fetch Column '%s' requested\n", columnName);

	columnBuf *colBuf;

	// Lock the name cache
	pthread_mutex_lock(&(colCache->nameCache.nameLock));

	// Check cache
	int idx = getColIdx(columnName);
	if (idx == -1) {
		// Fetching from disk will automatically lock the buffer for us

		printf("Column '%s' not in cache. Have to fetch from disk\n", columnName);
		colBuf = fetchColFromDisk(tbl, columnName, err);		// Fetch from disk
	} else {
		// Have to lock this buffer ourselves

		printf("Found column '%s' in cache at index %d\n", columnName, idx);
		colBuf = colCache->bufCache[idx];						// Fetch from cache

		if (colBuf == NULL) {
			err->err = ERR_INTERNAL;
			err->message = "Name cache and buffer cache out of sync. Column buffer was null";
		} else {
			pthread_mutex_lock(&(colBuf->colLock));
		}
	}

	// Release the name cache lock
	pthread_mutex_unlock(&(colCache->nameCache.nameLock));

	return colBuf;
}








