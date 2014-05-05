#include <stdio.h>

#include <columnTypes/btree/indexNode.h>
#include <mytypes.h>

int indexNodeCreate(indexNode **iNode, error *err) {
	*iNode = (indexNode *) malloc(sizeof(indexNode));
	if (*iNode == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

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
	(void) indexFp;
	(void) iNode;
	(void) offset;

	ERROR(err, E_UNIMP);
	return 1;
}

int indexNodeWrite(FILE *indexFp, indexNode *iNode, fileOffset_t *offset, error *err) {
	(void) indexFp;
	(void) iNode;
	(void) offset;

	ERROR(err, E_UNIMP);
	return 1;
}

void indexNodePrint(indexNode *iNode, const char *message) {
	printf("Index node: %s\n", message);

	printf("Is terminal: %s\n", (iNode->isTerminal) ? "true" : "false");
	printf("Num used keys: %d\n", iNode->nUsedKeys);
	
	printf("Keys: ");
	for (int i = 0; i < NUM_KEYS; i++) {
		printf("%d ", iNode->keys[i]);
	}
	printf("\n");

	printf("Children (file offsets): ");
	for (int i = 0; i < NUM_CHILDREN; i++) {
		printf("%d ", iNode->children[i]);
	}
	printf("\n");
}