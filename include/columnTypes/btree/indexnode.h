#ifndef _INDEXBLOCK_H_
#define _INDEXBLOCK_H_

#include <stdio.h>

#include <mytypes.h>
#include <error.h>

#define NUM_KEYS		2
#define NUM_CHILDREN	NUM_KEYS + 1

typedef struct indexNode {

	bool isTerminal;	// True -> children are data blocks, False -> children are index blocks

	int nUsedKeys;

	int keys[NUM_KEYS];
	fileOffset_t children[NUM_CHILDREN];

} indexNode;

int indexNodeCreate(indexNode **iNode, error *err);
void indexNodeDestroy(indexNode *iNode);

int indexNodeRead(FILE *indexFp, indexNode *iNode, fileOffset_t offset, error *err);
int indexNodeWrite(FILE *indexFp, indexNode *iNode, fileOffset_t *offset, error *err);

void indexNodePrint(indexNode *iNode, const char *message);

#endif