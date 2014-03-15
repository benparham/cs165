#include <stdio.h>
#include <string.h>

#include <columnTypes/btree.h>
#include <error.h>

void btreePrintHeader(columnHeaderBtree *header) {
	printf("Name: %s\nSize Bytes: %d\n", header->name, header->sizeBytes);
}

int btreeCreateHeader(columnHeaderBtree *header, char *columnName, error *err) {
	
	strcpy(header->name, columnName);
	header->sizeBytes = 0;
	return 0;
}

void btreeDestroyHeader(columnHeaderBtree *header) {
	free(header);
}

int btreeInsert(void *columnHeader, FILE *fp, char *data, error *err) {

	err->err = ERR_UNIMP;
	err->message = "Insert unimplemented for btree columns";
	return 1;
}