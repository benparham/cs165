#include <stdio.h>
#include <string.h>
#include <math.h>

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
	dBlock->nextBlock = 0;
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

	// indexNodePrint("Created new index node", iNode);

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
}

static int searchTerminalNode(FILE *indexFp, fileOffset_t nodeOffset, int data, indexNode *result, error *err) {

	// Read in node from file
	if (indexNodeRead(indexFp, result, nodeOffset, err)) {
		goto exit;
	}

	// indexNodePrint("Searching for terminal node", result);

	// Found a terminal node
	if (result->isTerminal) {
		return 0;
	}

	// Move on to correct child
	return searchTerminalNode(indexFp, getChildOffset(result, data), data, result, err);

exit:
	return 1;
}

static int addIndexLevel(FILE *indexFp, indexNode **_iNode, int keepIdx, error *err) {
	indexNode *iNode = *_iNode;

	MY_ASSERT(keepIdx >= 0 && keepIdx < NUM_CHILDREN);
	MY_ASSERT(indexNodeIsFull(iNode));
	MY_ASSERT(iNode->isTerminal);

	// Seek to the end of the file
	if (fseek(indexFp, 0, SEEK_END) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Location new nodes will be written to
	fileOffset_t appendOffset = ftell(indexFp);

	// Create a new node for each child
	indexNode* newNodes[NUM_CHILDREN];


	int i;
	for (i = 0; i < NUM_CHILDREN; i++) {
		if (indexNodeCreate(&(newNodes[i]), err)) {
			goto cleanupNewNodes;
		}

		fileOffset_t newOffset = appendOffset + (i * sizeof(indexNode));

		// Set data blocks as lone children of new nodes
		newNodes[i]->offset = newOffset;
		newNodes[i]->isTerminal = true;
		newNodes[i]->nUsedKeys = 0;
		newNodes[i]->children[0] = iNode->children[i];

		// Set new nodes as children of old node
		iNode->children[i] = newOffset;
	}

	// Old node is no longer terminal
	iNode->isTerminal = false;

	// Write out old node
	if (indexNodeWrite(indexFp, iNode, err)) {
		goto cleanupNewNodes;
	}

	// Write out and clean up new nodes
	bool failed = false;
	for (i = 0; i < NUM_CHILDREN; i++) {
		if (indexNodeWrite(indexFp, newNodes[i], err)) {
			failed = true;
		}

		if (i == keepIdx) {
			indexNodeDestroy(iNode);
			*_iNode = newNodes[i];
		} else {
			indexNodeDestroy(newNodes[i]);
		}
	}

	if (failed) {goto exit;}

	return 0;

cleanupNewNodes:
	while (i > 0) {
		i -= 1;
		indexNodeDestroy(newNodes[i]);
	}
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

	// TODO: change this to create function
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

	// TODO: change this to create function
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
			// ERROR(err, E_UNIMP);
			// goto cleanupBlock;

			// Add another level of index nodes
			if (addIndexLevel(header->indexFp, &iNode, childIdx, err)) {
				goto cleanupBlock;
			}

			// Update the current child index (addIndexLevel updates iNode)
			childIdx = 0;

			// Update header info
			header->fileIndexSizeBytes += NUM_CHILDREN * sizeof(indexNode);
			header->nNodes += NUM_CHILDREN;
		}

		// If the parent index node has free space...
		// else {

			// Split the data block, adding the new data
			dataBlock *newBlock;
			if (dataBlockSplitAdd(dBlock, &newBlock, data, err)) {
				goto cleanupBlock;
			}

			// Get offset for new block
			if (dataBlockSetAppendOffset(dataFp, newBlock, err)) {
				free(newBlock);
				goto cleanupBlock;
			}

			// Make blocks point to correct neighbors
			newBlock->nextBlock = (dataBlockIsEnd(dBlock)) ? newBlock->offset : dBlock->nextBlock;
			dBlock->nextBlock = newBlock->offset;

			// Write out updated blocks to data file
			if (dataBlockWrite(dataFp, dBlock, err) || dataBlockWrite(dataFp, newBlock, err)) {
				free(newBlock);
				goto cleanupBlock;
			}

			// Add new data block as one of index node's children
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
		// }
	}

	// Update header metadata
	header->nEntries += 1;

	// TODO: change this to destroy functions
	// Cleanup
	free(dBlock);
	free(iNode);

	// indexNodePrintAll("Done with insert", header->indexFp);

	return 0;

// TODO: change to destroy functions
cleanupBlock:
	free(dBlock);
cleanupNode:
	free(iNode);
exit:
	return 1;
}

int btreeSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Selecting all from btree column '%s'\n", header->name);

	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		goto exit;
	}

	if (bitmapCreate(header->nEntries, bmp, err)) {
		goto exit;
	}

	// dataBlock *dBlock;
	// if (dataBlockCreate(&dBlock, err)) {
	// 	goto exit;
	// }

	// for (int blockIdx = 0; blockIdx < header->nDataBlocks; blockIdx++) {
	// 	int blockOffset = dataBlockIdxToOffset(blockIdx);

	// 	if (dataBlockRead(dataFp, dBlock, blockOffset, err)) {
	// 		goto cleanupBlock;
	// 	}

	// 	int startBmpIdx = blockIdx * DATABLOCK_CAPACITY;
	// 	for (int i = 0; i < dBlock->nUsedEntries; i++) {
	// 		if (bitmapMark(*bmp, startBmpIdx + i, err)) {
	// 			goto cleanupBlock;
	// 		}
	// 	}
	// }

	// // Cleanup
	// dataBlockDestroy(dBlock);

	bitmapMarkAll(*bmp);

	return 0;

// cleanupBlock:
// 	dataBlockDestroy(dBlock);
exit:
	return 1;
}

static int getStartBmpIdx(FILE *dataFp, fileOffset_t firstDataBlock, dataBlock *dBlock, int *bmpIdx, error *err) {
	
	// Allocate space for a data block in memory
	dataBlock *tempBlock;
	if (dataBlockCreate(&tempBlock, err)) {
		goto exit;
	}

	// Sum up total number of entries before dBlock
	fileOffset_t curOffset = firstDataBlock;
	*bmpIdx = 0;
	while (curOffset != dBlock->offset) {
		// Read in next block
		if (dataBlockRead(dataFp, tempBlock, curOffset, err)) {
			goto cleanupBlock;
		}

		*bmpIdx += tempBlock->nUsedEntries;
		curOffset = tempBlock->nextBlock;
	}

	dataBlockDestroy(tempBlock);

	return 0;

cleanupBlock:
	dataBlockDestroy(tempBlock);
exit:
	return 1;
}

int btreeSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Selecting %d from btree column '%s'\n", value, header->name);

	// Check that column isn't empty
	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		goto exit;
	}

	// Allocate the bitmap
	if (bitmapCreate(header->nEntries, bmp, err)) {
		goto exit;
	}

	// Allocate index node
	indexNode *iNode;
	if (indexNodeCreate(&iNode, err)) {
		goto cleanupBitmap;
	}

	// Allocate data block
	dataBlock *dBlock;
	if (dataBlockCreate(&dBlock, err)) {
		goto cleanupNode;
	}

	// Get the correct terminal iNode
	if (searchTerminalNode(header->indexFp, header->rootIndexNode, value, iNode, err)) {
		goto cleanupBlock;
	}

	// Get the correct data block
	fileOffset_t blockOffset = getChildOffset(iNode, value);
	if (dataBlockRead(dataFp, dBlock, blockOffset, err)) {
		goto cleanupBlock;
	}

	dataBlockPrint("Found data block where value would be", dBlock);

	int startBmpIdx;
	if (getStartBmpIdx(dataFp, header->firstDataBlock, dBlock, &startBmpIdx, err)) {
		goto cleanupBlock;
	}

	while (1) {
		// // Calculate the bmp index from block offset
		// int blockIdx = dataBlockOffsetToIdx(dBlock);
		// int startBmpIdx = blockIdx * DATABLOCK_CAPACITY;

		bool done = false;

		// Check block for value
		for (int i = 0; i < dBlock->nUsedEntries; i++) {
			int data = dBlock->data[i];
			if (data == value) {
				if (bitmapMark(*bmp, startBmpIdx + i, err)) {
					goto cleanupBlock;
				}
			} else if (data > value) {
				done = true;
				break;
			}
		}

		if (done || dataBlockIsEnd(dBlock)) {
			break;
		}

		// Move to next block if necessary
		if (dataBlockRead(dataFp, dBlock, dBlock->nextBlock, err)) {
			goto cleanupBlock;
		}
		startBmpIdx += dBlock->nUsedEntries;
	}

	// Cleanup
	dataBlockDestroy(dBlock);
	indexNodeDestroy(iNode);

	return 0;

cleanupBlock:
	dataBlockDestroy(dBlock);
cleanupNode:
	indexNodeDestroy(iNode);
cleanupBitmap:
	bitmapDestroy(*bmp);
exit:
	return 1;
}

int btreeSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Selecting range %d - %d from btree column '%s'\n", low, high, header->name);

	// Check that column isn't empty
	if (header->nEntries < 1) {
		ERROR(err, E_COLEMT);
		goto exit;
	}

	// Allocate the bitmap
	if (bitmapCreate(header->nEntries, bmp, err)) {
		goto exit;
	}

	// Allocate index node
	indexNode *iNode;
	if (indexNodeCreate(&iNode, err)) {
		goto cleanupBitmap;
	}

	// Allocate data block
	dataBlock *dBlock;
	if (dataBlockCreate(&dBlock, err)) {
		goto cleanupNode;
	}

	// Get the correct terminal iNode for low
	if (searchTerminalNode(header->indexFp, header->rootIndexNode, low, iNode, err)) {
		goto cleanupBlock;
	}

	// Get the correct data block for low
	fileOffset_t blockOffset = getChildOffset(iNode, low);
	if (dataBlockRead(dataFp, dBlock, blockOffset, err)) {
		goto cleanupBlock;
	}

	dataBlockPrint("Found data block where low value would be", dBlock);

	int startBmpIdx;
	if (getStartBmpIdx(dataFp, header->firstDataBlock, dBlock, &startBmpIdx, err)) {
		goto cleanupBlock;
	}

	while (1) {
		// // Calculate the bmp index from block offset
		// int blockIdx = dataBlockOffsetToIdx(dBlock);
		// int startBmpIdx = blockIdx * DATABLOCK_CAPACITY;

		bool done = false;

		// Check block for value
		for (int i = 0; i < dBlock->nUsedEntries; i++) {
			int data = dBlock->data[i];
			if (data >= low && data <= high) {
				if (bitmapMark(*bmp, startBmpIdx + i, err)) {
					goto cleanupBlock;
				}
			} else if (data > high) {
				done = true;
				break;
			}
		}

		if (done || dataBlockIsEnd(dBlock)) {
			break;
		}

		// Move to next block if necessary
		if (dataBlockRead(dataFp, dBlock, dBlock->nextBlock, err)) {
			goto cleanupBlock;
		}
		startBmpIdx += dBlock->nUsedEntries;
	}

	// Cleanup
	dataBlockDestroy(dBlock);
	indexNodeDestroy(iNode);

	return 0;

cleanupBlock:
	dataBlockDestroy(dBlock);
cleanupNode:
	indexNodeDestroy(iNode);
cleanupBitmap:
	bitmapDestroy(*bmp);
exit:
	return 1;
}

int btreeFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	// Check that bitmap is correct size
	if (bitmapSize(bmp) != header->nEntries) {
		ERROR(err, E_BADFTC);
		goto exit;
	}


	dataBlock *dBlock;
	if (dataBlockCreate(&dBlock, err)) {
		goto exit;
	}


	int resultBuf[BUFSIZE];
	int indexBuf[BUFSIZE];
	int resultIdx = 0;

	int bmpSize = bitmapSize(bmp);
	int bmpIdx = 0;
	int blockOffset = header->firstDataBlock;

	while(1) {
		if (dataBlockRead(dataFp, dBlock, blockOffset, err)) {
			goto cleanupBlock;
		}

		dataBlockPrint("Checking data block during fetch", dBlock);

		for (int i = 0; i < dBlock->nUsedEntries; i++) {
			MY_ASSERT(bmpIdx < bmpSize);
			if (bitmapIsSet(bmp, bmpIdx)) {
				resultBuf[resultIdx] = dBlock->data[i];
				indexBuf[resultIdx] = bmpIdx;
				resultIdx += 1;
			}

			bmpIdx += 1;
		}

		if (dataBlockIsEnd(dBlock)) {
			break;
		} else {
			blockOffset = dBlock->nextBlock;
		}
	}

	// int length = bitmapSize(bmp);
	// for (int i = 0; i < length; i++) {
	// 	if (resultOffset >= BUFSIZE) {
	// 		ERROR(err, E_NOMEM);
	// 		goto cleanupBlock;
	// 	}

	// 	if (bitmapIsSet(bmp, i)) {
	// 		int blockIdx = i / DATABLOCK_CAPACITY;
	// 		int blockOffset = dataBlockIdxToOffset(blockIdx);
	// 		int innerIdx = i % 4;

	// 		if (dataBlockRead(dataFp, dBlock, blockOffset, err)) {
	// 			goto cleanupBlock;
	// 		}

	// 		resultBuf[resultOffset] = dBlock->data[innerIdx];
	// 		resultOffset += 1;
	// 	}
	// }

	*resultBytes = resultIdx * sizeof(int);

	*results = (int *) malloc(*resultBytes);
	*indices = (int *) malloc(*resultBytes);
	if (*results == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupBlock;
	}
	if (*indices == NULL) {
		free(*results);
		ERROR(err, E_NOMEM);
		goto cleanupBlock;
	}
	memcpy(*results, resultBuf, *resultBytes);
	memcpy(*indices, indexBuf, *resultBytes);

	// Cleanup
	dataBlockDestroy(dBlock);

	return 0;

cleanupBlock:
	dataBlockDestroy(dBlock);
exit:
	return 1;
}

int btreeLoad(void *_header, FILE *dataFp, int dataBytes, int *data, error *err) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	printf("Loading btree column %s\n", header->name);

	// Allocate waiting and working blocks in memory
	dataBlock *workingBlock;
	dataBlock *waitingBlock;
	if (dataBlockCreate(&workingBlock, err)) {
		goto exit;
	}
	if (dataBlockCreate(&waitingBlock, err)) {
		dataBlockDestroy(workingBlock);
		goto exit;
	}

	// Calculate number of data values
	MY_ASSERT(dataBytes % sizeof(int) == 0);
	int nValues = dataBytes / (sizeof(int));



// ============= Write out the data blocks

	// Write out data into data blocks on disk
	bool firstBlock = true;
	int blockIdx = 0;
	int blockDataIdx = 0;
	for (int i = 0; i < nValues; i++) {

		// Add data into working block's data
		workingBlock->data[blockDataIdx] = data[i];

		blockDataIdx += 1;

		// Once working block is filled to capacity...
		if (blockDataIdx == DATABLOCK_CAPACITY) {
			fileOffset_t newOffset = blockIdx * sizeof(dataBlock);

			// Set the metadata for the working block
			workingBlock->offset = newOffset;
			workingBlock->nUsedEntries = DATABLOCK_CAPACITY;

			if (firstBlock) {
				firstBlock = false;
			} else {
				// Set the waiting block to point to working block
				waitingBlock->nextBlock = newOffset;

				// Write out the waiting block
				if (dataBlockWrite(dataFp, waitingBlock, err)) {
					ERROR(err, E_CORRUPT);
					goto cleanupBlocks;
				}
			}

			// Swap working and waiting blocks
			dataBlock *temp = workingBlock;
			workingBlock = waitingBlock;
			waitingBlock = temp;

			// Move to next block offset
			blockIdx += 1;
			blockDataIdx = 0;
		}
	}

	// Write out any blocks left hanging in memory
	fileOffset_t newOffset = blockIdx * sizeof(dataBlock);

	if (blockDataIdx == 0) {
		// Make waiting block point to itself and write it out
		waitingBlock->nextBlock = waitingBlock->offset;

		if (dataBlockWrite(dataFp, waitingBlock, err)) {
			ERROR(err, E_CORRUPT);
			goto cleanupBlocks;
		}
	} else {
		// Make both waiting and working blocks point to working block and write them out
		waitingBlock->nextBlock = newOffset;
		
		workingBlock->offset = newOffset;
		workingBlock->nextBlock = newOffset;
		workingBlock->nUsedEntries = blockDataIdx;

		if (dataBlockWrite(dataFp, waitingBlock, err) ||
			dataBlockWrite(dataFp, workingBlock, err)) {
			ERROR(err, E_CORRUPT);
			goto cleanupBlocks;
		}
	}

	// Cleanup blocks
	dataBlockDestroy(workingBlock);
	dataBlockDestroy(waitingBlock);

	// Update header data information
	header->nEntries = nValues;
	header->nDataBlocks = nValues / DATABLOCK_CAPACITY;
	if (nValues % DATABLOCK_CAPACITY != 0) {
		header->nDataBlocks += 1;
	}
	header->fileDataSizeBytes = header->nDataBlocks * sizeof(dataBlock);
	header->firstDataBlock = 0;

// ============== Write out the index nodes

	// Allocate space for index node in memory
	indexNode *iNode;
	if (indexNodeCreate(&iNode, err)) {
		ERROR(err, E_CORRUPT);
		goto exit;
	}

	// Write out first level of index nodes
	int nIndexNodes = 0;
	blockIdx = 0;
	while(blockIdx < header->nDataBlocks) {
		
		// Re-initialize index node for writing
		iNode->offset = nIndexNodes * sizeof(indexNode);
		iNode->isTerminal = true;
		iNode->nUsedKeys = 0;
		memset(iNode->keys, 0, NUM_KEYS * sizeof(int));
		memset(iNode->children, 0, NUM_CHILDREN * sizeof(fileOffset_t));

		// Fill index node with children, updating keys
		for (int i = 0; i < NUM_CHILDREN; i++) {

			if (blockIdx >= header->nDataBlocks) {
				break;
			}

			// Add child data block to index node
			iNode->children[i] = blockIdx * sizeof(dataBlock);

			// Add key to index node
			if (i > 0) {
				iNode->nUsedKeys += 1;
				int dataIdx = (blockIdx * DATABLOCK_CAPACITY) - 1;
				iNode->keys[i - 1] = data[dataIdx];
			}

			blockIdx += 1;
		}

		// Write out filled index node
		if (indexNodeWrite(header->indexFp, iNode, err)) {
			ERROR(err, E_CORRUPT);
			goto cleanupNode;
		}

		nIndexNodes += 1;
	}

	// Write out remaining levels of index nodes
	int totalIndexNodes = nIndexNodes;

	int iNodeLvl = 2;
	fileOffset_t oldLvlOffset = 0;								// Offset of first index node of previous level
	fileOffset_t nodeOffset = nIndexNodes * sizeof(indexNode);	// Offset to write next index node to

	while(nIndexNodes > 1) {

		int nodeIdx = 0;		// Index of current previous level index node
		int nIndexNodesNew = 0;	// Count of current level nodes (nIndexNodes is previous level's count)

		// Iterate over all index nodes of previous level
		while(nodeIdx < nIndexNodes) {
			
			// Re-initialize index node for writing
			iNode->offset = nodeOffset;
			iNode->isTerminal = false;
			iNode->nUsedKeys = 0;
			memset(iNode->keys, 0, NUM_KEYS * sizeof(int));
			memset(iNode->children, 0, NUM_CHILDREN * sizeof(fileOffset_t));

			// Fill index node with children, updating keys
			for (int i = 0; i < NUM_CHILDREN; i++) {

				if (nodeIdx >= nIndexNodes) {
					break;
				}

				// Add child node to index node
				iNode->children[i] = oldLvlOffset + (nodeIdx * sizeof(indexNode));

				// Add key to index node
				if (i > 0) {
					iNode->nUsedKeys += 1;

					int entriesPerChildNode = pow(NUM_CHILDREN, iNodeLvl - 1) * DATABLOCK_CAPACITY;
					int dataIdx = (nodeIdx * entriesPerChildNode) - 1;
					iNode->keys[i - 1] = data[dataIdx];
				}

				nodeIdx += 1;
			}

			// Write out filled index node
			if (indexNodeWrite(header->indexFp, iNode, err)) {
				ERROR(err, E_CORRUPT);
				goto cleanupNode;
			}

			nIndexNodesNew += 1;

			nodeOffset += sizeof(indexNode);
		}

		iNodeLvl += 1;

		nIndexNodes = nIndexNodesNew;
		totalIndexNodes += nIndexNodes;
	}


	// Update header index information
	header->nNodes = totalIndexNodes;
	header->fileIndexSizeBytes = totalIndexNodes * sizeof(indexNode);
	header->rootIndexNode = iNode->offset;


	// Cleanup node
	indexNodeDestroy(iNode);

	return 0;

cleanupNode:
	indexNodeDestroy(iNode);
	goto exit;
cleanupBlocks:
	dataBlockDestroy(workingBlock);
	dataBlockDestroy(waitingBlock);
exit:
	return 1;
}

void btreePrintData(void *_header, FILE *dataFp) {
	columnHeaderBtree *header = (columnHeaderBtree *) _header;

	// Print index nodes
	if (header->nNodes == 0) {
		printf("No index nodes\n");
	} else {
		printf("\n");
		indexNodePrintAll("", header->indexFp);
	}

	// Print data blocks
	if (header->nDataBlocks == 0) {
		printf("No data blocks\n");
	} else {
		printf("\n");
		dataBlockPrintAll("", dataFp, header->firstDataBlock);
	}
}
