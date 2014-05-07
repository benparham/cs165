#ifndef _DATABLOCK_H_
#define _DATABLOCK_H_

#include <stdio.h>

#include <mytypes.h>
#include <error.h>

#define DATABLOCK_CAPACITY		4

typedef struct dataBlock {

	fileOffset_t offset;
	fileOffset_t nextBlock;

	int nUsedEntries;

	int data[DATABLOCK_CAPACITY];

} dataBlock;

int dataBlockCreate(dataBlock **dBlock, error *err);
void dataBlockDestroy(dataBlock *dBlock);

int dataBlockRead(FILE *dataFp, dataBlock *dBlock, fileOffset_t offset, error *err);
int dataBlockWrite(FILE *dataFp, dataBlock *dBlock, error *err);

int dataBlockSetAppendOffset(FILE *dataFp, dataBlock *dBlock, error *err);

bool dataBlockIsFull(dataBlock *dBlock);
bool dataBlockIsEnd(dataBlock *dBlock);

int dataBlockOffsetToIdx(dataBlock *dBlock);
int dataBlockIdxToOffset(int idx);

/* 
 * Add data to data block
 * Fails if data block is full
 */
int dataBlockAdd(dataBlock *dBlock, int data);

/*
 * For use on a full data block
 * Spilt data block into two, each containing half the (still sorted) data
 */
int dataBlockSplitAdd(dataBlock *oldBlock, dataBlock **newBlock, int data, error *err);


void dataBlockPrint(const char *message, dataBlock *dBlock);
void dataBlockPrintAll(const char *message, FILE *dataFp, fileOffset_t firstDataBlockOffset);

#endif