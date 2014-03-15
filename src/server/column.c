#include <column.h>
#include <error.h>
#include <columnTypes/common.h>
#include <columnTypes/unsorted.h>

static int columnSetFunctions(column *col, error *err) {
	switch (col->storageType) {
		case COL_UNSORTED:
			col->funcs = &unsortedColumnFunctions;
			break;
		case COL_SORTED:
			err->err = ERR_UNIMP;
			err->message = "Sorted columns not yet implemented";
			return 1;
			break;
		case COL_BTREE:
			err->err = ERR_UNIMP;
			err->message = "B-tree columns not yet implemented";
			return 1;
			break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Unsupported column storage type";
			return 1;
	}

	return 0;
}

// Allocates a new column struct and initializes it
int columnCreate(char *columnName, COL_STORAGE_TYPE storageType, column *col, error *err) {
	


	err->err = ERR_UNIMP;
	err->message = "Not yet implemented";
	return 1;
}
void columnDestroy(column *col) {

}

int columnReadFromDisk(tableInfo *tbl, char *columnName, column *col, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Not yet implemented";
	return 1;
}

int columnWriteToDisk(tableInfo *tbl, column *col, error *err) {
	err->err = ERR_UNIMP;
	err->message = "Not yet implemented";
	return 1;
}

void columnPrint(column * col) {

}


int columnInsert(column *col, int data, error *err) {
	if (col->funcs->insert(col->columnHeader, col->fp, data, err)) {
		return 1;
	}

	if (col->funcs->writeOutHeader(col->columnHeader, col->fp, err)) {
		return 1;
	}

	return 0;
}

int columnSelectAll(column *col, struct bitmap **bmp, error *err) {
	return col->funcs->selectAll(col->columnHeader, col->fp, bmp, err);
}

int columnSelectValue(column *col, int value, struct bitmap **bmp, error *err) {
	return col->funcs->selectValue(col->columnHeader, col->fp, value, bmp, err);
}
int columnSelectRange(column *col, int low, int high, struct bitmap **bmp, error *err) {
	return col->funcs->selectRange(col->columnHeader, col->fp, low, high, bmp, err);
}
int columnFetch(column *col, struct bitmap *bmp, error *err) {
	return col->funcs->fetch(col->columnHeader, col->fp, bmp, err);
}