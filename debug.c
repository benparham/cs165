#include <stdio.h>

#include "debug.h"
#include "database.h"


void printdbTableInfo(dbTableInfo *tbl) {
	printf("\nTable:\n");

	printf("Name: %s\n", tbl->name);
	printf("Validity: %d\n", tbl->isValid);
	printf("Number of columns: %d\n", tbl->numColumns);
}

void printdbColumnInfo(dbColumnInfo *col) {
	printf("\nColumn:\n");

	printf("Name: %s\n", col->name);
	printf("Size: %d\n", col->size_bytes);
}