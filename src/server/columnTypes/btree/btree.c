#include <stdio.h>
#include <string.h>

#include <columnTypes/btree/btree.h>
#include <columnTypes/btree/indexnode.h>
#include <columnTypes/btree/datablock.h>
#include <columnTypes/common.h>
#include <error.h>
#include <bitmap.h>

#define INDEX_FL_NM						"index.bin"

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
	header->nNodes = 0;
	header->rootIndexNode = 0;

//========== Data info
	header->fileDataSizeBytes = 0;
	header->nDataBlocks = 0;
	header->nEntries = 0;
	header->firstDataBlock = 0;

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
	serialReadInt(slzr, &(header->nNodes));
	serialReadFileOffset(slzr, &(header->rootIndexNode));

	serialReadInt(slzr, &(header->fileDataSizeBytes));
	serialReadInt(slzr, &(header->nDataBlocks));
	serialReadInt(slzr, &(header->nEntries));
	serialReadFileOffset(slzr, &(header->firstDataBlock));

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
	serialAddSerialSizeFileOffset(slzr, header->rootIndexNode);

	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeFileOffset(slzr, header->firstDataBlock);

	serializerAllocSerial(slzr);

	serialWriteStr(slzr, header->name);
	serialWriteStr(slzr, header->pathToDir);

	serialWriteInt(slzr, header->fileIndexSizeBytes);
	serialWriteInt(slzr, header->nNodes);
	serialWriteFileOffset(slzr, header->rootIndexNode);

	serialWriteInt(slzr, header->fileDataSizeBytes);
	serialWriteInt(slzr, header->nDataBlocks);
	serialWriteInt(slzr, header->nEntries);
	serialWriteFileOffset(slzr, header->firstDataBlock);

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
	printf("Number of nodes: %d\n", header->nNodes);
	printf("Root index node: %d\n", header->rootIndexNode);

	printf("File data size bytes: %d\n", header->fileDataSizeBytes);
	printf("Number of data blocks: %d\n", header->nDataBlocks);
	printf("Number of entries: %d\n", header->nEntries);
	printf("First data block: %d\n", header->firstDataBlock);
}

// Header info
	char *name;
	char *pathToDir;
	int fileHeaderSizeBytes;
	
	// Index info
	FILE *indexFp;
	int fileIndexSizeBytes;
	int nNodes;
	fileOffset_t rootIndexNode;

	// Data info
	int fileDataSizeBytes;
	int nDataBlocks;
	int nEntries;
	fileOffset_t firstDataBlock;

int btreeInsert(void *_header, FILE *dataFp, int data, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Inserting %d into btree column '%s'\n", data, header->name);

	/*
	 * Keep copies of the new header stats, only update the header struct once
	 * the changes they describe are pushed to disk
	 */
	int _fileIndexSizeBytes = header->fileIndexSizeBytes;
	int _nNodes = header->nNodes;

	// Holder for current working index node
	indexNode *curNode;

	// Get the root index node
	if (_nNodes == 0) {
		if (indexNodeCreate(&curNode, err)) {
			goto exit;
		}

		_fileIndexSizeBytes += sizeof(indexNode);
		_nNodes += 1;

		indexNodePrint(curNode, "Created new index node");
	} else {
		if (indexNodeRead(header->indexFp, &curNode, header->rootIndexNode, err)) {
			goto exit;
		}

		indexNodePrint(curNode, "Read in root index node");
	}

	

	indexNodeDestroy(curNode);

	return 0;

exit:
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