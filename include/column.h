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

	/*
	 * Various functions for interacting with a column. See columnTypes/common.h
	 * for details.
	 */
	columnFunctions *funcs;

} column;

int columnCreate(char *columnName, COL_STORAGE_TYPE storageType, column *col, error *err);
void columnDestroy(column *col);

int columnReadFromDisk(tableInfo *tbl, char *columnName, column *col, error *err);
int columnWriteToDisk(tableInfo *tbl, column *col, error *err);

void columnPrint(column *col);

int columnInsert(column *col, int data, error *err);
int columnSelectAll(column *col, struct bitmap **bmp, error *err);
int columnSelectValue(column *col, int value, struct bitmap **bmp, error *err);
int columnSelectRange(column *col, int low, int high, struct bitmap **bmp, error *err);
int columnFetch(column *col, struct bitmap *bmp, error *err);

#endif