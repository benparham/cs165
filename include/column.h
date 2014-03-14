#ifndef _COLUMN_H_
#define _COLUMN_H_

#include <stdio.h>

#include <global.h>
#include <table.h>
#include <error.h>

typedef enum {
	COL_UNSORTED,
	COL_SORTED,
	COL_BTREE
} COL_STORAGE_TYPE;

typedef enum {
	COL_INT
} COL_DATA_TYPE;

// // For use in determining the size to allocate for COL_DATA_TYPE data
// typedef union colDataType {
// 	int int_type;
// } colDataType;

typedef struct columnInfo {
	char name[NAME_SIZE];
	int sizeBytes;

	COL_STORAGE_TYPE storageType;
	// COL_DATA_TYPE dataType;
} columnInfo;

typedef struct columnBuf {
	columnInfo colInfo;
	FILE *fp;
	// unsigned char *data;
	// pthread_mutex_t colLock;
} columnBuf;

int strToColStorage(char *str, COL_STORAGE_TYPE *type);
int getColDataSize(COL_DATA_TYPE);

void printColumnInfo(columnInfo *col);

int columnBufCreate(columnBuf **colBuf);
void columnBufDestroy(columnBuf *colBuf);
int getColumnFromDisk(tableInfo *tbl, char *columnName, char *mode, columnBuf *colBuf, error *err);

#endif