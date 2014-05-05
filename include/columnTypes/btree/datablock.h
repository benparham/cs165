#ifndef _DATABLOCK_H_
#define _DATABLOCK_H_

#include <stdio.h>

#include <mytypes.h>
#include <error.h>

#define DATABLOCK_CAPACITY		4

typedef struct dataBlock {

	fileOffset_t offset;

	int nUsedEntries;

	fileOffset_t leftBlock;
	fileOffset_t rightBlock;

	int data[DATABLOCK_CAPACITY];

} dataBlock;

int dataBlockCreate(dataBlock **dBlock, error *err);
void dataBlockDestroy(dataBlock *dBlock);

int dataBlockRead(FILE *dataFp, dataBlock *dBlock, fileOffset_t offset, error *err);
int dataBlockWrite(FILE *dataFp, dataBlock *dBlock/*, fileOffset_t offset*/, error *err);
int dataBlockAppend(FILE *dataFp, dataBlock *dBlock, fileOffset_t *offset, error *err);

 
/* 
 * Add data to data block
 * Fails if data block is full
 */
int dataBlockAdd(dataBlock *dBlock, int data);

/*
 * For use on a full data block
 * Spilt data block into two, each containing half the (still sorted) data
 */
int dataBlockSplit();

/*
 * For use on a full data block
 * Add data to data block pushing extra data to neighbor
 * Fails if both neighbors are full
 */
int dataBlockShift();

void dataBlockPrint(dataBlock *dBlock, const char *message);

#endif