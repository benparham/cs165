#include <stdio.h>

#include "table.h"

void printtableInfo(tableInfo *tbl) {
	printf("\nTable:\n");

	printf("Name: %s\n", tbl->name);
	printf("Validity: %d\n", tbl->isValid);
	printf("Number of columns: %d\n", tbl->numColumns);
}