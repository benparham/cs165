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
	int  (* createHeader) 	(void **columnHeader, char *columnName, char *pathToDir, error *err);
	void (* destroyHeader) 	(void *columnHeader);
	int  (* readInHeader) 	(void **columnHeader, FILE *headerFp, error *err);
	int  (* writeOutHeader) (void *columnHeader, FILE *headerFp, error *err);
	void (* printHeader) 	(void *columnHeader);

	// Data functions
	int  (* insert) 	 (void *columnHeader, FILE *dataFp, int data, error *err);
	int  (* selectAll) 	 (void *columnHeader, FILE *dataFp, struct bitmap **bmp, error *err);
	int  (* selectValue) (void *columnHeader, FILE *dataFp, int value, struct bitmap **bmp, error *err);
	int  (* selectRange) (void *columnHeader, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err);
	int  (* fetch) 		 (void *columnHeader, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err);
	int  (* load) 		 (void *columnHeader, FILE *dataFp, int dataBytes, int *data, error *err);
	void (* printData)	 (void *columnHeader, FILE *dataFP);

} columnFunctions;

int seekHeader(FILE *headerFp, error *err);

int commonFetch(FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err);
int commonLoad(FILE *dataFp, int dataBytes, int *data, error *err);

#endif