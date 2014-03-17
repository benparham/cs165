#include <stdio.h>
#include <string.h>

#include <columnTypes/sorted.h>
#include <error.h>
#include <bitmap.h>

void sortedPrintHeader(columnHeaderSorted *header) {
	printf("Name: %s\nSize Bytes: %d\n", header->name, header->sizeBytes);
}

int sortedCreateHeader(columnHeaderSorted *header, char *columnName, error *err) {
	
	strcpy(header->name, columnName);
	header->sizeBytes = 0;
	return 0;
}

void sortedDestroyHeader(columnHeaderSorted *header) {
	free(header);
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

int sortedFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}