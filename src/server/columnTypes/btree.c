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

	int pathBytes = (strlen(pathToDir) + 1) * sizeof(char);
	header->pathToDir = (char *) malloc(pathBytes);
	if (header->pathToDir == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupName;
	}
	strcpy(header->pathToDir, pathToDir);

	header->fileHeaderSizeBytes = 0; // This will be set during readInHeader

//========== Index info
	char indexFilePath[BUFSIZE];
	sprintf(indexFilePath, "%s/%s", pathToDir, INDEX_FL_NM);

	// Create the column's index file
	header->indexFp = fopen(indexFilePath, "wb");
	if (header->indexFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupPath;
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
cleanupPath:
	free(header->pathToDir);
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
	free(header->pathToDir);

	fclose(header->indexFp);

	free(header);	
}

int btreeReadInHeader(void **_header, FILE *headerFp, error *err) {
	
	// Allocate header
	*_header = malloc(sizeof(columnHeaderBtree));
	if (*_header == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	columnHeaderBtree *header = (columnHeaderBtree *) *_header;

	// Seek to header location in file
	if (seekHeader(headerFp, err)) {
		goto cleanupHeader;
	}

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		ERROR(err, E_NOMEM);
		goto cleanupHeader;
	}

	// Read in serial from memory
	if (serializerSetSerialFromFile(slzr, headerFp)) {
		ERROR(err, E_SRL);
		goto cleanupSerial;
	}

	// Use the serial size to tell the header its own serial size
	header->fileHeaderSizeBytes = slzr->serialSizeBytes;

	serialReadStr(slzr, &(header->name));
	serialReadStr(slzr, &(header->pathToDir));

	char indexPath[BUFSIZE];
	sprintf(indexPath, "%s/%s", header->pathToDir, INDEX_FL_NM);

	header->indexFp = fopen(indexPath, "rb+");
	if (header->indexFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupSerial;
	}

	serialReadInt(slzr, &(header->fileIndexSizeBytes));
	serialReadInt(slzr, &(header->keysPerNode));
	serialReadInt(slzr, &(header->nNodes));

	serialReadInt(slzr, &(header->fileDataSizeBytes));
	serialReadInt(slzr, &(header->entriesPerDataBlock));
	serialReadInt(slzr, &(header->nDataBlocks));
	serialReadInt(slzr, &(header->nEntries));

	serializerDestroy(slzr);

	return 0;

// cleanupIndexFile:
// 	fclose(header->indexFp);
cleanupSerial:
	serializerDestroy(slzr);
cleanupHeader:
	free(header);
exit:
	return 1;
}

int btreeWriteOutHeader(void *_header, FILE *headerFp, error *err) {

	columnHeaderBtree *header = (columnHeaderBtree *) _header;

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
	serialAddSerialSizeStr(slzr, header->pathToDir);

	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);

	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);

	serializerAllocSerial(slzr);

	serialWriteStr(slzr, header->name);
	serialWriteStr(slzr, header->pathToDir);

	serialWriteInt(slzr, header->fileIndexSizeBytes);
	serialWriteInt(slzr, header->keysPerNode);
	serialWriteInt(slzr, header->nNodes);

	serialWriteInt(slzr, header->fileDataSizeBytes);
	serialWriteInt(slzr, header->entriesPerDataBlock);
	serialWriteInt(slzr, header->nDataBlocks);
	serialWriteInt(slzr, header->nEntries);

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

void btreePrintHeader(void *_header) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Name: %s\n", header->name);
	printf("Directory: %s\n", header->pathToDir);
	printf("File header size bytes: %d\n", header->fileHeaderSizeBytes);

	printf("File index size bytes: %d\n", header->fileIndexSizeBytes);
	printf("Keys per node: %d\n", header->keysPerNode);
	printf("Number of nodes: %d\n", header->nNodes);

	printf("File data size bytes: %d\n", header->fileDataSizeBytes);
	printf("Entries per data block: %d\n", header->entriesPerDataBlock);
	printf("Number of data blocks: %d\n", header->nDataBlocks);
	printf("Number of entries: %d\n", header->nEntries);
}

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