#include <stdio.h>
#include <string.h>

#include <columnTypes/unsorted.h>
#include <error.h>
#include <bitmap.h>

columnFunctions unsortedColumnFunctions = {
	// Header Functions
	&unsortedCreateHeader,
	&unsortedDestroyHeader,
	&unsortedReadInHeader,
	&unsortedWriteOutHeader,
	&unsortedPrintHeader,

	// Data functions
	&unsortedInsert,
	&unsortedSelectAll,
	&unsortedSelectValue,
	&unsortedSelectRange,
	&unsortedFetch
};

int unsortedCreateHeader(void *_header, char *columnName, error *err) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	strcpy(header->name, columnName);
	header->sizeBytes = 0;
	return 0;
}

void unsortedDestroyHeader(void *header) {
	free(header);
}

int unsortedReadInHeader(void *_header, FILE *fp, error *err) {
	// columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;
	err->err = ERR_UNIMP;
	err->message = "Not yet implemented";
	return 1;
}

int unsortedWriteOutHeader(void *_header, FILE *fp, error *err) {
	// columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;
	err->err = ERR_UNIMP;
	err->message = "Not yet implemented";
	return 1;
}

void unsortedPrintHeader(void *_header) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;
	printf("Name: %s\nSize Bytes: %d\n", header->name, header->sizeBytes);
}

// void unsortedSerializeHeader(columnHeaderUnsorted *header, void **serial, int *serialBytes) {
// 	*serial = header;
// 	*serialBytes = sizeof(columnHeaderUnsorted);
// }

int unsortedInsert(void *_header, FILE *fp, int data, error *err) {

	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

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

int unsortedSelectAll(void *_header, FILE *fp, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select all unimplemented for unsorted columns";
	return 1;
}

int unsortedSelectValue(void *_header, FILE *fp, int value, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select value unimplemented for unsorted columns";
	return 1;
}

int unsortedSelectRange(void *_header, FILE *fp, int low, int high, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select range unimplemented for unsorted columns";
	return 1;
}

int unsortedFetch(void *_header, FILE *fp, struct bitmap *bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Fetch unimplemented for unsorted columns";
	return 1;
}