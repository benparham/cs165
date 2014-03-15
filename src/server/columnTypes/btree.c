#include <stdio.h>
#include <string.h>

#include <columnTypes/btree.h>
#include <error.h>
#include <bitmap.h>

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

int btreeInsert(void *columnHeader, FILE *fp, int data, error *err) {

	err->err = ERR_UNIMP;
	err->message = "Insert unimplemented for btree columns";
	return 1;
}

int btreeSelectAll(void *columnHeader, FILE *fp, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select all unimplemented for btree columns";
	return 1;
}

int btreeSelectValue(void *columnHeader, FILE *fp, int value, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select value unimplemented for btree columns";
	return 1;
}

int btreeSelectRange(void *columnHeader, FILE *fp, int low, int high, struct bitmap **bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Select range unimplemented for btree columns";
	return 1;
}

int btreeFetch(void *columnHeader, FILE *fp, struct bitmap *bmp, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Fetch unimplemented for btree columns";
	return 1;
}