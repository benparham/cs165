#ifndef _UNSORTED_H_
#define _UNSORTED_H_

#include <stdio.h>

#include <global.h>
#include <error.h>
#include <bitmap.h>
#include <columnTypes/common.h>

extern columnFunctions unsortedColumnFunctions;

// TODO: Change name to a char *
typedef struct columnHeaderUnsorted {
	char name[NAME_SIZE];
	int sizeBytes;
	int nEntries;
} columnHeaderUnsorted;

int unsortedCreateHeader(void **_header, char *columnName, error *err);
void unsortedDestroyHeader(void *_header);
int unsortedReadInHeader(void **_header, FILE *headerFp, error *err);
int unsortedWriteOutHeader(void *_header, FILE *headerFp, error *err);
void unsortedPrintHeader(void *_header);

int unsortedInsert(void *_header, FILE *dataFp, int data, error *err);
int unsortedSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err);
int unsortedSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err);
int unsortedSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err);
int unsortedFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, error *err);

#endif