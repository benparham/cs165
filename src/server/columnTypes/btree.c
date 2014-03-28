#include <stdio.h>
#include <string.h>

#include <columnTypes/btree.h>
#include <columnTypes/common.h>
#include <error.h>
#include <bitmap.h>

#define INDEX_FL_NM						"index.bin"

#define DEFAULT_KEYS_PER_NODE			2
#define DEFAULT_ENTRIES_PER_DATABLOCK	4

columnFunctions btreeColumnFunctions = {
	// Header Functions
	&btreeCreateHeader,
	&btreeDestroyHeader,
	&btreeReadInHeader,
	&btreeWriteOutHeader,
	&btreePrintHeader,

	// Data functions
	&btreeInsert,
	&btreeSelectAll,
	&btreeSelectValue,
	&btreeSelectRange,
	&btreeFetch,
	&btreeLoad
};


int btreeCreateHeader(void **_header, char *columnName, char *pathToDir, error *err) {

	*_header = malloc(sizeof(columnHeaderBtree));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	columnHeaderBtree *header = (columnHeaderBtree *) *_header;

//========== Header info
	int nameBytes = (strlen(columnName) + 1) * sizeof(char);
	header->name = (char *) malloc(nameBytes);
	if (header->name == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupHeader;
	}
	strcpy(header->name, columnName);

	header->fileHeaderSizeBytes = 0; // This will be set during readInHeader


//========== Index info
	char indexFilePath[BUFSIZE];
	sprintf(indexFilePath, "%s/%s", pathToDir, INDEX_FL_NM);

	// Create the column's index file
	header->indexFp = fopen(indexFilePath, "wb");
	if (header->indexFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupName;
	}

	header->fileIndexSizeBytes = 0;
	header->keysPerNode = DEFAULT_KEYS_PER_NODE;
	header->nNodes = 0;

//========== Data info
	header->fileDataSizeBytes = 0;
	header->entriesPerDataBlock = DEFAULT_ENTRIES_PER_DATABLOCK;
	header->nDataBlocks = 0;
	header->nEntries = 0;

	return 0;

// cleanupIndexFile:
// 	fclose(header->indexFp);
// 	remove(indexFilePath);
cleanupName:
	free(header->name);
cleanupHeader:
	free(header);
exit:
	return 1;
}

void btreeDestroyHeader(void *_header) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	free(header->name);

	fclose(header->indexFp);

	free(header);	
}

int btreeReadInHeader(void **_header, FILE *headerFp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeWriteOutHeader(void *_header, FILE *headerFp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

void btreePrintHeader(void *_header) {}

int btreeInsert(void *_header, FILE *dataFp, int data, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int btreeLoad(void *_header, FILE *dataFp, int dataBytes, int *data, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}