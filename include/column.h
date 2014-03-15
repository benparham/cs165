#ifndef _COLUMN_H_
#define _COLUMN_H_

#include <stdio.h>

// #include <global.h>
#include <table.h>
#include <error.h>	

typedef enum {
	COL_UNSORTED,
	COL_SORTED,
	COL_BTREE
} COL_STORAGE_TYPE;

int strToColStorage(char *str, COL_STORAGE_TYPE *type);

// typedef struct columnInfo {
// 	char name[NAME_SIZE];
// 	int sizeBytes;

// 	COL_STORAGE_TYPE storageType;
// } columnInfo;

// typedef struct columnBuf {
// 	columnInfo colInfo;
// 	FILE *fp;
// } columnBuf;

// ================ Abstract Column Interface ==================
typedef struct column {
	FILE *fp;							// Pointer to column's file
	COL_STORAGE_TYPE storageType;		// Storage type of column
	
	/*
	 * Info about column located immediately after storage type in column's file
	 * Column header varies according to storage type
	 */
	void *columnHeader;

	// Functions
	int (* insert) (void *columnHeader, FILE *fp, char *data, error *err);

} column;

int columnHeaderByteSize(COL_STORAGE_TYPE storageType);

void columnPrintHeader(COL_STORAGE_TYPE storageType, void *header);
void columnPrint(column *col);

// Header management
int columnCreateNewHeader(COL_STORAGE_TYPE storageType, char *columnName, void **header, int *sizeBytes, error *err);
void columnDestroyHeader(COL_STORAGE_TYPE storageType, void *header);

// Column management
int columnCreateFromDisk(tableInfo *tbl, char *columnName, column *col, error *err);
void columnDestroy(column *col);

// Abstract column functions
int columnInsert(column *col, char *data, error *err);


// void printColumnInfo(columnInfo *col);

// int columnBufCreate(columnBuf **colBuf);
// void columnBufDestroy(columnBuf *colBuf);
// int getColumnFromDisk(tableInfo *tbl, char *columnName, char *mode, columnBuf *colBuf, error *err);

#endif