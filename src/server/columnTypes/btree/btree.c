#include <stdio.h>
#include <string.h>
// #include <assert.h>

#include <columnTypes/btree/btree.h>
#include <columnTypes/btree/indexnode.h>
#include <columnTypes/btree/datablock.h>
#include <columnTypes/common.h>
#include <error.h>
#include <bitmap.h>
#include <global.h>

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
	&btreeLoad,
	&btreePrintData
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

static int createFirstNode(columnHeaderBtree *header, FILE *dataFp, int data, error *err) {
	MY_ASSERT(header->nNodes == 0);
	MY_ASSERT(header->fileIndexSizeBytes == 0);
	MY_ASSERT(header->nDataBlocks == 0);
	MY_ASSERT(header->fileDataSizeBytes == 0);

	indexNode *iNode;
	dataBlock *dBlock;

	// Create a new index node and data block
	if (indexNodeCreate(&iNode, err)) {
		goto exit;
	}

	if (dataBlockCreate(&dBlock, err)) {
		goto cleanupNode;
	}

	// Set index node data
	iNode->offset = 0;
	iNode->children[0] = 0;

	// Set data block data
	dBlock->offset = 0;
	dBlock->nUsedEntries = 1;
	dBlock->leftBlock = 0;
	dBlock->rightBlock = 0;
	dBlock->data[0] = data;

	// Write out data block and index node
	if (indexNodeWrite(header->indexFp, iNode, err)) {
		goto cleanupBlock;
	}
	if (dataBlockWrite(dataFp, dBlock, err)) {
		goto cleanupNodeWrite;
	}

	// Update header information
	header->rootIndexNode = 0;
	header->firstDataBlock = 0;

	header->nNodes += 1;
	header->fileIndexSizeBytes += sizeof(indexNode);

	header->nDataBlocks += 1;
	header->nEntries += 1;
	header->fileDataSizeBytes += sizeof(dataBlock);

	indexNodePrint("Created new index node", iNode);

	// Cleanup
	free(dBlock);
	free(iNode);

	return 0;

cleanupNodeWrite:
	memset(iNode, 0, sizeof(indexNode));
	iNode->offset = 0;
	if (indexNodeWrite(header->indexFp, iNode, err)) {
		ERROR(err, E_CORRUPT);
	}
cleanupBlock:
	free(dBlock);
cleanupNode:
	free(iNode);
exit:
	return 1;
}

static int getChildIdx(indexNode *iNode, int data) {
	// Only has one child
	if (iNode->nUsedKeys == 0) {
		return 0;
	} 

	// Search keys for correct child
	else {
		int i;
		for (i = 0; i < iNode->nUsedKeys; i++) {
			if (data <= iNode->keys[i]) {
				return i;
			}
		}

		return i;
	}
}

static fileOffset_t getChildOffset(indexNode *iNode, int data) {
	return iNode->children[getChildIdx(iNode, data)];

	// // Only has one child
	// if (iNode->nUsedKeys == 0) {
	// 	return iNode->children[0];
	// } 

	// // Search keys for correct child
	// else {
	// 	int i;
	// 	for (i = 0; i < iNode->nUsedKeys; i++) {
	// 		if (data <= iNode->keys[i]) {
	// 			return iNode->children[i];
	// 		}
	// 	}

	// 	return iNode->children[i];
	// }
}

static int searchTerminalNode(FILE *indexFp, fileOffset_t nodeOffset, int data, indexNode *result, error *err) {

	// Read in node from file
	if (indexNodeRead(indexFp, result, nodeOffset, err)) {
		goto exit;
	}

	indexNodePrint("Searching for terminal node", result);

	// Found a terminal node
	if (result->isTerminal) {
		return 0;
	}

	// Move on to correct child
	return searchTerminalNode(indexFp, getChildOffset(result, data), data, result, err);

exit:
	return 1;
}

int btreeInsert(void *_header, FILE *dataFp, int data, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Inserting %d into btree column '%s'\n", data, header->name);

	// If no index exists, special case of creating first node
	if (header->nNodes == 0) {
		if (createFirstNode(header, dataFp, data, err)) {
			goto exit;
		}
		return 0;
	}


	// Allocate space for working index node
	indexNode *iNode = (indexNode *) malloc(sizeof(indexNode));
	if (iNode == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	// Get correct terminal index node for data
	if (searchTerminalNode(header->indexFp, header->rootIndexNode, data, iNode, err)) {
		goto cleanupNode;
	}
	MY_ASSERT(iNode != NULL && iNode->isTerminal);

	// Allocate space for working data block
	dataBlock *dBlock = (dataBlock *) malloc(sizeof(dataBlock));
	if (dBlock == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupNode;
	}

	int childIdx = getChildIdx(iNode, data);
	int childOffset = getChildOffset(iNode, data);

	// Get correct data block for data
	if (dataBlockRead(dataFp, dBlock, childOffset, err)) {
		goto cleanupBlock;
	}

	dataBlockPrint("Data block for insert", dBlock);

	// If there's space in the data block...
	if (!dataBlockIsFull(dBlock)) {
		
		// Add data to data block
		if (dataBlockAdd(dBlock, data)) {
			ERROR(err, E_INTERN);
			goto cleanupBlock;
		}

		// Write out data block and index node
		if (indexNodeWrite(header->indexFp, iNode, err) ||
			dataBlockWrite(dataFp, dBlock, err)) {
			goto cleanupBlock;
		}
	}

	// If there's no space in the data block...
	else {
		// If the parent index node is full...
		if (indexNodeIsFull(iNode)) {
			ERROR(err, E_UNIMP);
			goto cleanupBlock;
		}

		// If the parent index node has free space...
		else {

			// Split the data block, adding the new data
			dataBlock *newBlock;
			if (dataBlockSplitAdd(dBlock, &newBlock, data, err)) {
				goto cleanupBlock;
			}

			// Make new block point to correct places
			newBlock->rightBlock = dBlock->rightBlock;
			newBlock->leftBlock = dBlock->offset;

			// Write out new block to data file
			if (dataBlockAppend(dataFp, newBlock, err)) {
				free(newBlock);
				goto cleanupBlock;
			}

			// Make old block point to new block
			dBlock->rightBlock = newBlock->offset;

			// Write out old block to data file
			if (dataBlockWrite(dataFp, dBlock, err)) {
				free(newBlock);
				goto cleanupBlock;
			}

			// Add new data block as one of parent index's children
			int key = dBlock->data[dBlock->nUsedEntries - 1];
			if (indexNodeAdd(iNode, newBlock, key, childIdx, err)) {
				free(newBlock);
				ERROR(err, E_CORRUPT);
				goto cleanupBlock;
			}

			// Write index node to index file
			if (indexNodeWrite(header->indexFp, iNode, err)) {
				goto cleanupBlock;
			}

			// Update relevent header metadata
			header->nDataBlocks += 1;
			header->fileDataSizeBytes += sizeof(dataBlock);
		}
	}

	// Update header metadata
	header->nEntries += 1;

	// Cleanup
	free(dBlock);
	free(iNode);

	indexNodePrintAll("Done with insert", header->indexFp);

	return 0;

cleanupBlock:
	free(dBlock);
cleanupNode:
	free(iNode);
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

void btreePrintData(void *_header, FILE *dataFp) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	// Print index node
	indexNodePrintAll("", header->indexFp);

	// Print data blocks
	dataBlockPrintAll("", dataFp, header->firstDataBlock);
}
