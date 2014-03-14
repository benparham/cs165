#ifndef _SORTED_H_
#define _SORTED_H_

#include <global.h>
#include <error.h>

typedef struct columnHeaderSorted {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderSorted;

void sortedPrintHeader(columnHeaderSorted *header);

int sortedCreateHeader(columnHeaderSorted *header, char *columnName, error *err);
void sortedDestroyHeader(columnHeaderSorted *header);

int sortedInsert(void *columnHeader, FILE *fp, char *data, error *err);

#endif