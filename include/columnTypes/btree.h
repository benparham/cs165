#ifndef _BTREE_H_
#define _BTREE_H_

#include <global.h>
#include <error.h>
#include <bitmap.h>

typedef struct columnHeaderBtree {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderBtree;

void btreePrintHeader(columnHeaderBtree *header);

int btreeCreateHeader(columnHeaderBtree *header, char *columnName, error *err);
void btreeDestroyHeader(columnHeaderBtree *header);

int btreeInsert(void *columnHeader, FILE *fp, int data, error *err);
int btreeSelectAll(void *columnHeader, FILE *fp, struct bitmap **bmp, error *err);
int btreeSelectValue(void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err);
int btreeSelectRange(void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err);
int btreeFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, error *err);

#endif