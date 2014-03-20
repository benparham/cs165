#include <stdio.h>
#include <string.h>

#include <columnTypes/sorted.h>
#include <columnTypes/common.h>
#include <error.h>
#include <bitmap.h>

columnFunctions sortedColumnFunctions = {
	// Header Functions
	&sortedCreateHeader,
	&sortedDestroyHeader,
	&sortedReadInHeader,
	&sortedWriteOutHeader,
	&sortedPrintHeader,

	// Data functions
	&sortedInsert,
	&sortedSelectAll,
	&sortedSelectValue,
	&sortedSelectRange,
	&sortedFetch
};

int sortedCreateHeader(void **_header, char *columnName, error *err) {
	
	*_header = malloc(sizeof(columnHeaderSorted));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}

	columnHeaderSorted *header = (columnHeaderSorted *) *_header;

	int nameBytes = (strlen(columnName) + 1) * sizeof(char);
	header->name = (char *) malloc(nameBytes);
	if (header->name == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}
	strcpy(header->name, columnName);

	header->fileHeaderSizeBytes = 0;	// This will be set during readInHeader
	header->fileDataSizeBytes = sizeof(int);
	// header->sizeBytes = sizeof(int);

	header->entriesTotal = 1;
	header->entriesUsed = 0;

	if (bitmapCreate(1, &(header->bmp), err)) {
		return 1;
	}

	return 0;
}

void sortedDestroyHeader(void *_header) {
	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	free(header->name);
	bitmapDestroy(header->bmp);

	free(_header);
}

int sortedReadInHeader(void **_header, FILE *headerFp, error *err) {
	
	ERROR(err, E_UNIMP);
	return 1;

	// Allocate header
	*_header = malloc(sizeof(columnHeaderSorted));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	columnHeaderSorted *header = (columnHeaderSorted *) *_header;

	// Seek to header location in file
	if (seekHeader(headerFp, err)) {
		goto exit;
	}

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	// Set up serial buffer
	unsigned char serial[BUFSIZE];
	memset(serial, 0, BUFSIZE);

	// Read in serial from memory
	if (serializerSetSerialFromFile(slzr, headerFp)) {
		ERROR(err, E_SRL);
		goto cleanupSerial;
	}

	// Use the serial size to tell the header its own serial size
	header->fileHeaderSizeBytes = slzr->serialSizeBytes;

	// Deserialize for all other header info
	serialReadStr(slzr, &(header->name));
	serialReadInt(slzr, &(header->fileDataSizeBytes));
	serialReadInt(slzr, &(header->entriesTotal));
	serialReadInt(slzr, &(header->entriesUsed));
	bitmapSerialRead(slzr, &(header->bmp));

	serializerDestroy(slzr);

	return 0;

cleanupSerial:
	serializerDestroy(slzr);
exit:
	return 1;
}

int sortedWriteOutHeader(void *_header, FILE *headerFp, error *err) {

	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	// Seek to header location in file
	if (seekHeader(headerFp, err)) {
		goto exit;
	}

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	// Serialize header
	serialAddSerialSizeStr(slzr, header->name);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	bitmapSerialAddSize(slzr, header->bmp);

	serializerAllocSerial(slzr);

	serialWriteStr(slzr, header->name);
	serialWriteInt(slzr, header->fileDataSizeBytes);
	serialWriteInt(slzr, header->entriesTotal);
	serialWriteInt(slzr, header->entriesUsed);
	bitmapSerialWrite(slzr, header->bmp);

	// Write serialized header to disk
	if (fwrite(slzr->serial, slzr->serialSizeBytes, 1, headerFp) < 1) {
		ERROR(err, E_FWR);
		goto cleanupSerial;
	}

	// Destroy serializer
	serializerDestroy(slzr);

	return 0;

cleanupSerial:
	serializerDestroy(slzr);
exit:
	return 1;
}

void sortedPrintHeader(void *_header) {
	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	printf("Name: %s\n", header->name);
	printf("File header size bytes: %d\n", header->fileHeaderSizeBytes);
	printf("File data size bytes: %d\n", header->fileDataSizeBytes);
	printf("Entries total: %d\n", header->entriesTotal);
	printf("Entries used: %d\n", header->entriesUsed);
	printf("Bitmap:\n");
	bitmapPrint(header->bmp);
}



int sortedInsert(void *columnHeader, FILE *dataFp, int data, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectAll(void *columnHeader, FILE *dataFp, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectValue(void *columnHeader, FILE *dataFp, int value, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedSelectRange(void *columnHeader, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int sortedFetch(void *columnHeader, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}