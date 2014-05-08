#include <stdio.h>
#include <string.h>

#include <columnTypes/unsorted.h>
#include <columnTypes/common.h>
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
	&unsortedFetch,
	&unsortedLoad,
	&unsortedPrintData
};

int unsortedCreateHeader(void **_header, char *columnName, char *pathToDir, error *err) {
	(void) pathToDir;

	*_header = malloc(sizeof(columnHeaderUnsorted));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
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

// TODO: header->name as a char * and use serializer here and in writeOutHeader
int unsortedReadInHeader(void **_header, FILE *headerFp, error *err) {
	*_header = malloc(sizeof(columnHeaderUnsorted));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}

	columnHeaderUnsorted *header = (columnHeaderUnsorted *) *_header;

	if (seekHeader(headerFp, err)) {
		return 1;
	}

	if (fread(header, sizeof(columnHeaderUnsorted), 1, headerFp) < 1) {
		ERROR(err, E_FRD);
		return 1;
	}

	return 0;
}

int unsortedWriteOutHeader(void *_header, FILE *headerFp, error *err) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (seekHeader(headerFp, err)) {
		return 1;
	}

	if (fwrite(header, sizeof(columnHeaderUnsorted), 1, headerFp) < 1) {
		ERROR(err, E_FWR);
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

int unsortedInsert(void *_header, FILE *dataFp, int data, error *err) {

	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	// Seek to the end of the file
	if (fseek(dataFp, 0, SEEK_END) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	// Write data to file
	if (fwrite(&data, sizeof(int), 1, dataFp) < 1) {
		ERROR(err, E_FWR);
		return 1;
	}

	// Update the header info
	header->sizeBytes += sizeof(int);
	header->nEntries += 1;

	return 0;
}

int unsortedSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err) {
	
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		return 1;
	}

	if (bitmapCreate(header->nEntries, bmp, err)) {
		return 1;
	}

	bitmapMarkAll(*bmp);

	return 0;
}

int unsortedSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err) {
	
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		return 1;
	}

	if (bitmapCreate(header->nEntries, bmp, err)) {
		return 1;
	}

	
	if (fseek(dataFp, 0, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	} 

	int length = header->nEntries;
	for (int i = 0; i < length; i++) {
		int entry;

		if (fread(&entry, sizeof(int), 1, dataFp) < 1) {
			ERROR(err, E_FRD);
			return 1;
		}

		if (entry == value) {
			if (bitmapMark(*bmp, i, err)) {
				return 1;
			}
		}
	} 

	return 0;
}

int unsortedSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err) {
	
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		return 1;
	}

	if (bitmapCreate(header->nEntries, bmp, err)) {
		return 1;
	}


	if (fseek(dataFp, 0, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	int length = header->nEntries;
	for (int i = 0; i < length; i++) {
		int entry;

		if (fread(&entry, sizeof(int), 1, dataFp) < 1) {
			ERROR(err, E_FRD);
			return 1;
		}

		if (entry >= low && entry <= high) {
			if (bitmapMark(*bmp, i, err)) {
				return 1;
			}
		}
	} 

	return 0;
}

int unsortedFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err) {
	if (bitmapSize(bmp) != ((columnHeaderUnsorted *) _header)->nEntries) {
		ERROR(err, E_BADFTC);
		return 1;
	}

	return commonFetch(dataFp, bmp, resultBytes, results, indices, err);
}

int unsortedLoad(void *_header, FILE *dataFp, int dataBytes, int *data, error *err) {
	columnHeaderUnsorted *header = (columnHeaderUnsorted *) _header;

	if (commonLoad(dataFp, dataBytes, data, err)) {
		return 1;
	}

	header->sizeBytes = dataBytes;
	header->nEntries = (dataBytes / sizeof(int));

	return 0;
}

void unsortedPrintData(void *_header, FILE *dataFp) {
	
}