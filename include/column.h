#ifndef _COLUMN_H_
#define _COLUMN_H_

#include <stdio.h>

#include <table.h>
#include <error.h>
#include <columnTypes/common.h>
#include <bitmap.h>


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
	int (* insert) (void *columnHeader, FILE *fp, int data, error *err);
	int (* selectAll) (void *columnHeader, FILE *fp, struct bitmap **bmp, error *err);
	int (* selectValue) (void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err);
	int (* selectRange) (void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
	int (* fetch) (void *columnHeader, FILE *fp, struct bitmap *bmp, error *err);

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
int columnInsert(column *col, int data, error *err);
int columnSelectAll(column *col, struct bitmap **bmp, error *err);
int columnSelectValue(column *col, int value, struct bitmap **bmp, error *err);
int columnSelectRange(column *col, int low, int high, struct bitmap **bmp, error *err);
int columnFetch(column *col, struct bitmap *bmp, error *err);

#endif