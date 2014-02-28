#ifndef _COLUMN_H_
#define _COLUMN_H_

#include "global.h"

typedef enum {
	COL_UNSORTED,
	COL_SORTED,
	COL_BTREE
} COL_STORAGE_TYPE;

typedef enum {
	COL_INT
} COL_DATA_TYPE;

typedef struct columnInfo {
	char name[NAME_SIZE];
	int sizeBytes;

	COL_STORAGE_TYPE storageType;
	COL_DATA_TYPE dataType;
} columnInfo;

int strToColStorage(char *str, COL_STORAGE_TYPE *type);
int getColDataSize(COL_DATA_TYPE);

void printcolumnInfo(columnInfo *col);

#endif