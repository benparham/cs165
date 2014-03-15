#include <stdio.h>
#include <string.h>

#include <columnTypes/unsorted.h>
#include <error.h>

void unsortedPrintHeader(columnHeaderUnsorted *header) {
	printf("Name: %s\nSize Bytes: %d\n", header->name, header->sizeBytes);
}

int unsortedCreateHeader(columnHeaderUnsorted *header, char *columnName, error *err) {
	strcpy(header->name, columnName);
	header->sizeBytes = 0;
	return 0;
}

void unsortedDestroyHeader(columnHeaderUnsorted *header) {
	free(header);
}

int unsortedInsert(void *columnHeader, FILE *fp, char *data, error *err) {

	// int toWrite = atoi(data);

	// if (fwrite())

	err->err = ERR_UNIMP;
	err->message = "Insert unimplemented for unsorted columns";
	return 1;
}