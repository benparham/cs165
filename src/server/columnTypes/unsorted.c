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

int unsortedInsert(void *columnHeader, FILE *fp, int data, error *err) {

	columnHeaderUnsorted *header = (columnHeaderUnsorted *) columnHeader;

	// Seek to the end of the file
	if (fseek(fp, 0, SEEK_END) == -1) {
		err->err = ERR_INTERNAL;
		err->message = "Failed to seek in column file";
		return 1;
	}

	// Write data to file
	if (fwrite(&data, sizeof(int), 1, fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write data to column file";
		return 1;
	}

	// Update the header info
	header->sizeBytes += sizeof(int);

	return 0;
}