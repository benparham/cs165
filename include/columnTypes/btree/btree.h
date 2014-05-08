#ifndef _BTREE_H_
#define _BTREE_H_

#include <global.h>
#include <myTypes.h>
#include <columnTypes/common.h>
#include <error.h>
#include <bitmap.h>

extern columnFunctions btreeColumnFunctions;

typedef struct columnHeaderBtree {
	
	// Header info
	char *name;
	char *pathToDir;
	int fileHeaderSizeBytes;
	
	// Index info
	FILE *indexFp;
	int fileIndexSizeBytes;
	int nNodes;
	fileOffset_t rootIndexNode;

	// Data info
	int fileDataSizeBytes;
	int nDataBlocks;
	int nEntries;
	fileOffset_t firstDataBlock;

} columnHeaderBtree;

int btreeCreateHeader(void **_header, char *columnName, char *pathToDir, error *err);
void btreeDestroyHeader(void *_header);
int btreeReadInHeader(void **_header, FILE *headerFp, error *err);
int btreeWriteOutHeader(void *_header, FILE *headerFp, error *err);
void btreePrintHeader(void *_header);

int btreeInsert(void *_header, FILE *dataFp, int data, error *err);
int btreeSelectAll(void *_header, FILE *dataFp, struct bitmap **bmp, error *err);
int btreeSelectValue(void *_header, FILE *dataFp, int value, struct bitmap **bmp, error *err);
int btreeSelectRange(void *_header, FILE *dataFp, int low, int high, struct bitmap **bmp, error *err);
int btreeFetch(void *_header, FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err);
int btreeLoad(void *_header, FILE *dataFp, int dataBytes, int *data, error *err);
void btreePrintData(void *_header, FILE *dataFp);

#endif