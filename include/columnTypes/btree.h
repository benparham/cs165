#ifndef _BTREE_H_
#define _BTREE_H_

#include <global.h>
#include <error.h>

typedef struct columnHeaderBtree {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderBtree;

void btreePrintHeader(columnHeaderBtree *header);

int btreeCreateHeader(columnHeaderBtree *header, char *columnName, error *err);
void btreeDestroyHeader(columnHeaderBtree *header);

int btreeInsert(void *columnHeader, FILE *fp, int data, error *err);

#endif