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

	(*dBlock)->offset = 0;			// Must be set properly before writing

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

	// Record the offset we read from
	dBlock->offset = offset;

	return 0;

exit:
	return 1;
}

int dataBlockWrite(FILE *dataFp, dataBlock *dBlock/*, fileOffset_t offset*/, error *err) {

	assert(dBlock != NULL);

	// Seek to the correct position in the file
	if (fseek(dataFp, dBlock->offset, SEEK_SET) == -1) {
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
	dBlock->offset = ftell(dataFp);

	// Write to the end
	if (dataBlockWrite(dataFp, dBlock, err)) {
		goto exit;
	}

	// Return the position written to
	*offset = dBlock->offset;

	return 0;

exit:
	return 1;
}


int dataBlockAdd(dataBlock *dBlock, int data) {
	MY_ASSERT(!(dBlock->nUsedEntries > DATABLOCK_CAPACITY));

	// Check for room
	if (dBlock->nUsedEntries == DATABLOCK_CAPACITY) {
		return 1;
	}

	// Find slot for data
	int idx;
	for (idx = 0; idx < dBlock->nUsedEntries; idx++) {
		if (data < dBlock->data[idx]) {
			break;
		}
	}

	// Shift data above slot up by one
	for (int j = dBlock->nUsedEntries; j > idx; j--) {
		dBlock->data[j] = dBlock->data[j - 1];
	}

	// Add data
	dBlock->data[idx] = data;

	// Update metadata
	dBlock->nUsedEntries += 1;

	return 0;
}


void dataBlockPrint(dataBlock *dBlock, const char *message) {
	printf("Data block: %s\n", message);

	printf("Num used entries: %d\n", dBlock->nUsedEntries);
	printf("Left block: %d\n", dBlock->leftBlock);
	printf("Right block: %d\n", dBlock->rightBlock);
	
	printf("Entries: \n");
	for (int i = 0; i < dBlock->nUsedEntries; i++) {
		printf("%d", dBlock->data[i]);
	}
	printf("\n");
}