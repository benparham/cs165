#ifndef _SORTED_H_
#define _SORTED_H_

#include <global.h>
#include <error.h>
#include <bitmap.h>
#include <columnTypes/common.h>

extern columnFunctions sortedColumnFunctions;

typedef struct columnHeaderSorted {
	char *name;
	int fileHeaderSizeBytes;

	int fileDataSizeBytes;
	int nEntries;

	// int entriesTotal;
	// int entriesUsed;

	// struct bitmap *bmp;
} columnHeaderSorted;

int sortedCreateHeader(void **_header, char *columnName, char *pathToDir, error *err);
void sortedDestroyHeader(void *_header);
int sortedReadInHeader(void **_header, FILE *headerFp, error *err);
int sortedWriteOutHeader(void *_header, FILE *headerFp, error *err);
void sortedPrintHeader(void *_header);

int sortedInsert(void *_header, FILE *dataFp, int data, error *err);
int sortedSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err);
int sortedSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err);
int sortedSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err);
int sortedFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, error *err);
int sortedLoad(void *_header, FILE *dataFp, int dataBytes, int *data, error *err);

#endif