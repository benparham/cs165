#ifndef _SORTED_H_
#define _SORTED_H_

#include <global.h>
#include <error.h>
#include <bitmap.h>

typedef struct columnHeaderSorted {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderSorted;

void sortedPrintHeader(columnHeaderSorted *header);

int sortedCreateHeader(columnHeaderSorted *header, char *columnName, error *err);
void sortedDestroyHeader(columnHeaderSorted *header);

int sortedInsert(void *columnHeader, FILE *fp, int data, error *err);
int sortedSelectAll(void *columnHeader, FILE *fp, struct bitmap **bmp, error *err);
int sortedSelectValue(void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err);
int sortedSelectRange(void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
int sortedFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, error *err);

#endif