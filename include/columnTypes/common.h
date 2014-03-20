#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>

#include <error.h>
#include <bitmap.h>

typedef enum {
	COL_UNSORTED,
	COL_SORTED,
	COL_BTREE
} COL_STORAGE_TYPE;

int strToColStorage(char *str, COL_STORAGE_TYPE *type);

typedef struct columnFunctions {

	// Header Functions
	int (* createHeader) (void **columnHeader, char *columnName, error *err);
	void (* destroyHeader) (void *columnHeader);
	int (* readInHeader) (void **columnHeader, FILE *fp, error *err);
	int (* writeOutHeader) (void *columnHeader, FILE *fp, error *err);
	void (* printHeader) (void *columnHeader);

	// Data functions
	int (* insert) (void *columnHeader, FILE *fp, int data, error *err);
	int (* selectAll) (void *columnHeader, FILE *fp, struct bitmap **bmp, error *err);
	int (* selectValue) (void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err);
	int (* selectRange) (void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
	int (* fetch) (void *columnHeader, FILE *fp, struct bitmap *bmp, int *resultBytes, int **results, error *err);

} columnFunctions;

int seekHeader(FILE *fp, int offset, error *err);
int seekData(FILE *fp, int fileHeaderSizeBytes, int offset, error *err);

int commonFetch(int fileHeaderSizeBytes, FILE *fp, struct bitmap *bmp, int *resultBytes, int **results, error *err);

#endif