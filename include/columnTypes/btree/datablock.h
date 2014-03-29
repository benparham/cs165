#ifndef _DATABLOCK_H_
#define _DATABLOCK_H_

#include <filesys.h>
#include <error.h>
#include <serial.h>

#define DATABLOCK_CAPACITY		4

typedef struct dataBlock {

	int nUsedEntries;

	fileOffset_t leftBlock;
	fileOffset_t rightBlock;

	int data[DATABLOCK_CAPACITY];

} dataBlock;

int dataBlockCreate(dataBlock **dBlock, error *err);
void dataBlockDestroy(dataBlock *dBlock);

int dataBlockSerialAddSize(serializer *slzr, dataBlock *dBlock, error *err);
int dataBlockSerialWrite(serializer *slzr, dataBlock *dBlock, error *err);
int dataBlockSerialRead(serializer *slzr, dataBlock **dBlock, error *err);

/* 
 * Inserts data into the desired block. Either of the block's two neighbors may
 * also be modified. Returns the updated lowest and highest entries in the block
 * via *low and *high. If block and both neighbors are all full, returns *full == 1
 * and does nothing.
 */
int dataBlockAdd(fileOffset_t blockOffset, int data, int *full, int *low, int *high, error *err);

int dataBlockSplit();

#endif