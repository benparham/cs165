#include <stdio.h>
#include <string.h>

#include <columnTypes/btree/indexNode.h>
#include <mytypes.h>

int indexNodeCreate(indexNode **iNode, error *err) {
	*iNode = (indexNode *) malloc(sizeof(indexNode));
	if (*iNode == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	(*iNode)->offset = 0; 			// Must be set properly before writing
	(*iNode)->isTerminal = true;
	(*iNode)->nUsedKeys = 0;

	memset((*iNode)->keys, 0, NUM_KEYS * sizeof(int));
	memset((*iNode)->children, 0, NUM_CHILDREN * sizeof(fileOffset_t));

	return 0;

exit:
	return 1;
}

void indexNodeDestroy(indexNode *iNode) {
	free(iNode);
}

int indexNodeRead(FILE *indexFp, indexNode *iNode, fileOffset_t offset, error *err) {
	
	// Seek to the correct position in the file
	if (fseek(indexFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Read in the index node
	if (fread(iNode, sizeof(indexNode), 1, indexFp) < 1) {
		ERROR(err, E_FRD);
		goto exit;
	}

	// Recored the offset we read from
	iNode->offset = offset;

	return 0;

exit:
	return 1;
}

int indexNodeWrite(FILE *indexFp, indexNode *iNode, error *err) {

	// Seek to the correct position in the file
	if (fseek(indexFp, iNode->offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Write out data block
	if (fwrite(iNode, sizeof(indexNode), 1, indexFp) <= 0) {
		ERROR(err, E_FWR);
		goto exit;
	}

	return 0;

exit:
	return 1;
}

bool indexNodeIsFull(indexNode *iNode) {
	MY_ASSERT(!(iNode->nUsedKeys > NUM_KEYS));
	return (iNode->nUsedKeys == NUM_KEYS);
}

int indexNodeAdd(indexNode *iNode, dataBlock *dBlock, int key, int keyIdx, error *err) {

	if (indexNodeIsFull(iNode)) {
		ERROR(err, E_INTERN);
		goto exit;
	}

	int childIdx = keyIdx + 1;

	int lastChildIdx = iNode->nUsedKeys;
	int lastKeyIdx = lastChildIdx - 1;

	int i = lastKeyIdx;
	int j = lastChildIdx;

	// Shift existing keys and children offsets over
	while(i >= keyIdx) {
		iNode->keys[i + 1] = iNode->keys[i];
		iNode->children[j + 1] = iNode->children[j];

		i -= 1;
		j -= 1;
	}

	// Add new key and child offset
	iNode->keys[keyIdx] = key;
	iNode->children[childIdx] = dBlock->offset;

	return 0;

exit:
	return 1;
}

void indexNodePrint(const char *message, indexNode *iNode) {
	printf("Index node: %s\n", message);

	printf("Is terminal: %s\n", (iNode->isTerminal) ? "true" : "false");
	printf("Num used keys: %d\n", iNode->nUsedKeys);
	
	printf("Keys: ");
	for (int i = 0; i < iNode->nUsedKeys; i++) {
		printf("%d ", iNode->keys[i]);
	}
	printf("\n");

	printf("Children (file offsets): ");
	for (int i = 0; i < iNode->nUsedKeys + 1; i++) {
		printf("%d ", iNode->children[i]);
	}
	printf("\n");
}

void indexNodePrintAll(const char *message, FILE *indexFp) {
	
	indexNode *iNode = (indexNode *) malloc(sizeof(indexNode));
	if (iNode == NULL) {
		goto exit;
	}

	printf("Entire Index: %s\n", message);

	// Seek to the correct position in the file
	if (fseek(indexFp, 0, SEEK_SET) == -1) {
		goto exit;
	}

	while (fread(iNode, sizeof(indexNode), 1, indexFp) == 1) {
		printf("_____________\n");
		indexNodePrint("", iNode);		
	}

	return;

exit:
	printf("Error printing btree index nodes\n");
	return;
}
