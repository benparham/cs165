#include <stdio.h>
#include <string.h>

#include <columnTypes/sorted.h>
#include <error.h>
#include <bitmap.h>

void sortedPrintHeader(void *_header) {
	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	printf("Name: %s\n", header->name);
	printf("Size bytes: %d\n", header->sizeBytes);
	printf("Entries total: %d\n", header->entriesTotal);
	printf("Entries used: %d\n", header->entriesUsed);
	bitmapPrint(header->bmp);
}

int sortedCreateHeader(void **_header, char *columnName, error *err) {
	
	*_header = malloc(sizeof(columnHeaderSorted));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}

	columnHeaderSorted *header = (columnHeaderSorted *) *_header;

	strcpy(header->name, columnName);
	header->sizeBytes = 0;

	header->entriesTotal = 0;
	header->entriesUsed = 0;

	// bitmapCreate

	return 0;
}

void sortedDestroyHeader(void *_header) {
	free(_header);
}

int sortedInsert(void *columnHeader, FILE *fp, int data, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectAll(void *columnHeader, FILE *fp, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectValue(void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectRange(void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, int *resultBytes, int **results, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}