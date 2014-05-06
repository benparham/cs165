#ifndef _INDEXBLOCK_H_
#define _INDEXBLOCK_H_

#include <stdio.h>

#include <mytypes.h>
#include <error.h>
#include <columnTypes/btree/datablock.h>

#define NUM_KEYS		2
#define NUM_CHILDREN	NUM_KEYS + 1

typedef struct indexNode {

	fileOffset_t offset;

	bool isTerminal;	// True -> children are data blocks, False -> children are index blocks

	int nUsedKeys;

	int keys[NUM_KEYS];
	fileOffset_t children[NUM_CHILDREN];

} indexNode;

int indexNodeCreate(indexNode **iNode, error *err);
void indexNodeDestroy(indexNode *iNode);

int indexNodeRead(FILE *indexFp, indexNode *iNode, fileOffset_t offset, error *err);
int indexNodeWrite(FILE *indexFp, indexNode *iNode, error *err);

bool indexNodeIsFull(indexNode *iNode);

// Add data block to the index node's list of children, updating keys as necessary
int indexNodeAdd(indexNode *iNode, dataBlock *dBlock, int key, int idx, error *err);

void indexNodePrint(indexNode *iNode, const char *message);
int indexPrint(const char *message, FILE *indexFp, error *err);

#endif