#include <columnTypes/btree/datablock.h>
#include <filesys.h>
#include <error.h>

int dataBlockCreate(dataBlock **dBlock, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

void dataBlockDestroy(dataBlock *dBlock) {}



int dataBlockSerialAddSize(serializer *slzr, dataBlock *dBlock, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int dataBlockSerialWrite(serializer *slzr, dataBlock *dBlock, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}

int dataBlockSerialRead(serializer *slzr, dataBlock **dBlock, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}



int dataBlockAdd(fileOffset_t blockOffset, int data, int *full, int *low, int *high, error *err) {
	ERROR(err, E_UNIMP);
	return 1;
}