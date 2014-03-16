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

int unsortedCreateHeader(void **_header, char *columnName, error *err) {
	*_header = malloc(sizeof(columnHeaderUnsorted));
	if (*_header == NULL) {
		err->err = ERR_MEM;
		err->message = "Failed to allocate unsorted column header";
		return 1;
	}

	strcpy(((columnHeaderUnsorted *) *_header)->name, columnName);
	((columnHeaderUnsorted *) *_header)->sizeBytes = 0;
	((columnHeaderUnsorted *) *_header)->nEntries = 0;
	return 0;
}

void unsortedDestroyHeader(void *header) {
	free(header);
}

int unsortedReadInHeader(void *_header, FILE *fp, error *err) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;
	
	if (seekToHeader(fp, err)) {
		return 1;
	}

	if (fread(header, sizeof(columnHeaderUnsorted), 1, fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Failed to read from column file";
		return 1;
	}

	return 0;
}

int unsortedWriteOutHeader(void *_header, FILE *fp, error *err) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (seekToHeader(fp, err)) {
		return 1;
	}

	if (fwrite(header, sizeof(columnHeaderUnsorted), 1, fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Failed to write to column file";
		return 1;
	}

	return 0;
}

void unsortedPrintHeader(void *_header) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	printf("Name: %s\n", header->name);
	printf("Size bytes: %d\n", header->sizeBytes);
	printf("Number of entries: %d\n", header->nEntries);
}

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
	header->nEntries += 1;

	return 0;
}

int unsortedSelectAll(void *_header, FILE *fp, struct bitmap **bmp, error *err) {
	
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (header->nEntries < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Cannot select from column. It is empty.";
		return 1;
	}

	if (bitmapCreate(header->nEntries, bmp)) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create bitmap";
		return 1;
	}

	if (bitmapMarkAll(*bmp)) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to mark bitmap";
		return 1;
	}

	return 0;
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