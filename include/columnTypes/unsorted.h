#ifndef _UNSORTED_H_
#define _UNSORTED_H_

#include <stdio.h>

#include <global.h>
#include <error.h>
#include <bitmap.h>
#include <columnTypes/common.h>

extern columnFunctions unsortedColumnFunctions;

typedef struct columnHeaderUnsorted {
	char name[NAME_SIZE];
	int sizeBytes;
	int nEntries;
} columnHeaderUnsorted;

int unsortedCreateHeader(void **_header, char *columnName, error *err);
void unsortedDestroyHeader(void *_header);
int unsortedReadInHeader(void *_header, FILE *fp, error *err);
int unsortedWriteOutHeader(void *_header, FILE *fp, error *err);
void unsortedPrintHeader(void *_header);
// void unsortedSerializeHeader(columnHeaderUnsorted *header, void **serial, int *serialBytes);

int unsortedInsert(void *_header, FILE *fp, int data, error *err);
int unsortedSelectAll(void *_header, FILE *fp, struct bitmap **bmp, error *err);
int unsortedSelectValue(void *_header, FILE *fp, int value, struct bitmap **bmp, error *err);
int unsortedSelectRange(void *_header, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
int unsortedFetch(void *_header, FILE *fp, struct bitmap *bmp, error *err);

#endif