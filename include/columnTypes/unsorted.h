#ifndef _UNSORTED_H_
#define _UNSORTED_H_

#include <stdio.h>

#include <global.h>
#include <error.h>

typedef struct columnHeaderUnsorted {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderUnsorted;

void unsortedPrintHeader(columnHeaderUnsorted *header);

int unsortedCreateHeader(columnHeaderUnsorted *header, char *columnName, error *err);
void unsortedDestroyHeader(columnHeaderUnsorted *header);

int unsortedInsert(void *columnHeader, FILE *fp, int data, error *err);

#endif