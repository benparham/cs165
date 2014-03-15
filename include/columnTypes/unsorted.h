#ifndef _UNSORTED_H_
#define _UNSORTED_H_

#include <stdio.h>

#include <global.h>
#include <error.h>
#include <bitmap.h>

typedef struct columnHeaderUnsorted {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderUnsorted;

void unsortedPrintHeader(columnHeaderUnsorted *header);

int unsortedCreateHeader(columnHeaderUnsorted *header, char *columnName, error *err);
void unsortedDestroyHeader(columnHeaderUnsorted *header);

int unsortedInsert(void *columnHeader, FILE *fp, int data, error *err);
int unsortedSelectAll(void *columnHeader, FILE *fp, struct bitmap **bmp, error *err);
int unsortedSelectValue(void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err);
int unsortedSelectRange(void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
int unsortedFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, error *err);

#endif