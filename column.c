#include <stdio.h>
#include <string.h>

#include "column.h"

int getColDataSize(COL_DATA_TYPE type) {
	switch (type) {
		case COL_INT:
			return 4;
		default:
			return -1;
	}
}

int strToColStorage(char *str, COL_STORAGE_TYPE *type) {
	int result = 1;

	if (strcmp(str, "unsorted") == 0) {
		*type = COL_UNSORTED;
		result = 0;
	} else if (strcmp(str, "sorted") == 0) {
		*type = COL_SORTED;
		result = 0;
	} else if (strcmp(str, "btree") == 0) {
		*type = COL_BTREE;
		result = 0;
	}

	return result;
}

void printColumnInfo(columnInfo *col) {
	printf("\nColumn:\n");

	printf("Name: %s\n", col->name);
	printf("Size: %d\n", col->sizeBytes);

	printf("Storage type: %d\n", col->storageType);
	printf("Data type: %d\n", col->dataType);
}