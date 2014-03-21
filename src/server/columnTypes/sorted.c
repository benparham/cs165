#include <stdio.h>
#include <string.h>
#include <assert.h>

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
	header->fileDataSizeBytes = 0;
	header->nEntries = 0;

	// header->entriesTotal = 1;
	// header->entriesUsed = 0;

	// if (bitmapCreate(1, &(header->bmp), err)) {
	// 	return 1;
	// }

	return 0;
}

void sortedDestroyHeader(void *_header) {
	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	free(header->name);
	// bitmapDestroy(header->bmp);

	free(_header);
}

int sortedReadInHeader(void **_header, FILE *headerFp, error *err) {

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
	serialReadInt(slzr, &(header->nEntries));
	// serialReadInt(slzr, &(header->entriesTotal));
	// serialReadInt(slzr, &(header->entriesUsed));
	// bitmapSerialRead(slzr, &(header->bmp));

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
	// serialAddSerialSizeInt(slzr);
	// serialAddSerialSizeInt(slzr);
	// bitmapSerialAddSize(slzr, header->bmp);

	serializerAllocSerial(slzr);

	serialWriteStr(slzr, header->name);
	serialWriteInt(slzr, header->fileDataSizeBytes);
	serialWriteInt(slzr, header->nEntries);
	// serialWriteInt(slzr, header->entriesTotal);
	// serialWriteInt(slzr, header->entriesUsed);
	// bitmapSerialWrite(slzr, header->bmp);

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
	// printf("Entries total: %d\n", header->entriesTotal);
	// printf("Entries used: %d\n", header->entriesUsed);
	// printf("Bitmap:\n");
	// bitmapPrint(header->bmp);
}


// Returns 1 on error, 0 on "success"
static int binSrch(FILE *dataFp, int value, int idxLow, int idxHigh, int *idxRet, int *valRet, error *err) {
	if (dataFp == NULL || idxRet == NULL || valRet == NULL ||
		idxLow > idxHigh || idxHigh == 0) {
		ERROR(err, E_INTERN);
		return 1;
	}


	int idxMid = idxLow + ((idxHigh - idxLow) / 2);
	int offset = idxMid * sizeof(int);

	if (fseek(dataFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	int entry;
	if (fread(&entry, sizeof(int), 1, dataFp) < 1) {
		ERROR(err, E_FRD);
		return 1;
	}

	if (value == entry || idxHigh - idxLow <= 2) {
		*idxRet = idxMid;
		*valRet = entry;
		return 0;
	}

	if (value < entry) {
		return binSrch(dataFp, value, idxLow, idxMid, idxRet, valRet, err);
	} else {
		return binSrch(dataFp, value, idxMid, idxHigh, idxRet, valRet, err);
	}
}



int sortedInsert(void *_header, FILE *dataFp, int data, error *err) {
	columnHeaderSorted *header = (columnHeaderSorted *) _header;

	// Get insertion index
	int insertIdx;
	if (header->nEntries == 0) {
		insertIdx = 0;
	} else {
		int lastIdx = header->nEntries - 1;

		int srchVal;
		if (binSrch(dataFp, data, 0, lastIdx, &insertIdx, &srchVal, err)) {
			return 1;
		}

		if (data > srchVal) {
			insertIdx += 1;
		}
	}

	printf("Need to insert at index %d\n", insertIdx);

	// Seek to index's offset in file
	int offset = insertIdx * sizeof(int);
	if (fseek(dataFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	// Bytes remaining in the file after offset
	int bytesToCopy = header->fileDataSizeBytes - offset;
	assert(bytesToCopy >= 0);

	if (bytesToCopy > BUFSIZE) {
		ERROR(err, E_NOMEM);
		return 1;
	}
	unsigned char temp[BUFSIZE];

	// Read data after offset into temp
	if (fread(temp, bytesToCopy, 1, dataFp) < 1) {
		ERROR(err, E_FRD);
		return 1;
	}


	// Write data to the correct index
	if (fseek(dataFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}
	if (fwrite(&data, sizeof(int), 1, dataFp) < 1) {
		ERROR(err, E_FWR);
		return 1;
	}

	// Write temp back into file after data
	if (fwrite(temp, bytesToCopy, 1, dataFp) < 1) {
		ERROR(err, E_FWR);
		return 1;
	}


	// Update the column header
	header->fileDataSizeBytes += sizeof(int);
	header->nEntries += 1;

	return 0;
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