#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <columnTypes/btree/datablock.h>
#include <mytypes.h>
#include <error.h>

// Constants for special offset "pointers" to other blocks
#define ROOT_BLOCK				-1
#define LAST_BLOCK				-2
#define NO_BLOCK				-3

int dataBlockCreate(dataBlock **dBlock, error *err) {
	*dBlock = (dataBlock *) malloc(sizeof(dataBlock));
	if (*dBlock == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	(*dBlock)->nUsedEntries = 0;
	
	(*dBlock)->leftBlock = NO_BLOCK;
	(*dBlock)->rightBlock = NO_BLOCK;

	memset((*dBlock)->data, 0, DATABLOCK_CAPACITY * sizeof(int));

	return 0;
	
exit:
	return 1;
}

void dataBlockDestroy(dataBlock *dBlock) {
	free(dBlock);
}


int dataBlockRead(FILE *dataFp, dataBlock *dBlock, fileOffset_t offset, error *err) {

	// Seek to the correct position in the file
	if (fseek(dataFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Read in the data block
	if (fread(dBlock, sizeof(dataBlock), 1, dataFp) < 1) {
		ERROR(err, E_FRD);
		goto exit;
	}

	return 0;

exit:
	return 1;
}

int dataBlockWrite(FILE *dataFp, dataBlock *dBlock, fileOffset_t offset, error *err) {

	assert(dBlock != NULL);

	// Seek to the correct position in the file
	if (fseek(dataFp, offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Write out data block
	if (fwrite(dBlock, sizeof(dataBlock), 1, dataFp) <= 0) {
		ERROR(err, E_FWR);
		goto exit;
	}

	return 0;

exit:
	return 1;
}

int dataBlockAppend(FILE *dataFp, dataBlock *dBlock, fileOffset_t *offset, error *err) {
	
	// Seek to the end of the file
	if (fseek(dataFp, 0, SEEK_END) == -1) {
		ERROR(err, E_FSK);
		goto exit;
	}

	// Get the current position (in bytes)
	int pos = ftell(dataFp);

	// Write to the end
	if (dataBlockWrite(dataFp, dBlock, pos, err)) {
		goto exit;
	}

	// Return the position written to
	*offset = pos;

	return 0;

exit:
	return 1;
}

// int dataBlockSerialAddSize(serializer *slzr, dataBlock *dBlock, error *err) {
	
// 	serialAddSerialSizeInt(slzr); // nUsedEntries
// 	serialAddSerialSizeFileOffset(slzr, dBlock->leftBlock);
// 	serialAddSerialSizeFileOffset(slzr, dBlock->rightBlock);

// 	serialAddSerialSizeRaw(slzr, sizeof(dBlock->data)); // data
// 	printf("Sizeof(dBlock->data) = %lu\n", sizeof(dBlock->data));

// 	return 0;
// }

// int dataBlockSerialWrite(serializer *slzr, dataBlock *dBlock, error *err) {
	
// 	serialWriteInt(slzr, dBlock->nUsedEntries);
// 	serialWriteFileOffset(slzr, dBlock->leftBlock);
// 	serialWriteFileOffset(slzr, dBlock->rightBlock);

// 	serialWriteRaw(slzr, dBlock->data, sizeof(dBlock->data));

// 	return 0;
// }

// int dataBlockSerialRead(serializer *slzr, dataBlock **dBlock, error *err) {
	
// 	*dBlock = (dataBlock *) malloc(sizeof(dataBlock));
// 	if (*dBlock == NULL) {
// 		ERROR(err, E_NOMEM);
// 		goto exit;
// 	}

// 	serialReadInt(slzr, &((*dBlock)->nUsedEntries));
// 	serialReadFileOffset(slzr, &((*dBlock)->leftBlock));
// 	serialReadFileOffset(slzr, &((*dBlock)->rightBlock));

// 	int bytesRead;
// 	serialReadRaw(slzr, (void **) &((*dBlock)->data), &bytesRead);

// 	assert(bytesRead == DATABLOCK_CAPACITY * sizeof(int));

// 	return 0;

// exit:
// 	return 1;
// }



int dataBlockAdd(fileOffset_t blockOffset, int data, int *full, int *low, int *high, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}