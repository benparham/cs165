#include <stdio.h>
#include <string.h>

#include <columnTypes/sorted.h>
#include <error.h>

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

	err->err = ERR_UNIMP;
	err->message = "Insert unimplemented for sorted columns";
	return 1;
}