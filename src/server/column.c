#include <column.h>
#include <error.h>
#include <filesys.h>
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
int columnCreate(char *columnName, COL_STORAGE_TYPE storageType, FILE *fp, column **col, error *err) {

	*col = (column *) malloc(sizeof(column));
	if (*col == NULL) {
		err->err = ERR_MEM;
		err->message = "Failed to allocate a new column buffer";
		goto exit;
	}

	(*col)->storageType = storageType;
	(*col)->fp = fp;

	if (columnSetFunctions(*col, err)) {
		goto cleanupColumn;
	}

	if ((*col)->funcs->createHeader(&((*col)->columnHeader), columnName, err)) {
		goto cleanupColumn;
	}

	return 0;

cleanupColumn:
	free(*col);
exit:
	return 1;
}

void columnDestroy(column *col) {
	fclose(col->fp);
	col->funcs->destroyHeader(col->columnHeader);
	free(col);
}

// Requires and allocated column as argument
int columnReadFromDisk(tableInfo *tbl, char *columnName, column *col, error *err) {

	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	printf("Attempting to open column file %s\n", pathToColumn);

	if (!fileExists(pathToColumn)) {
		err->err = ERR_DUP;
		err->message = "Cannot read column. Does not exist";
		goto exit;
	}

	// Open the column file
	col->fp = fopen(pathToColumn, "rb+");
	if (col->fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file for column";
		goto exit;
	}

	// Read in the storage type
	if (fread(&(col->storageType), sizeof(COL_STORAGE_TYPE), 1, col->fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to read from column file";
		goto cleanupFile;
	}

	// Setup column functions
	if (columnSetFunctions(col, err)) {
		goto cleanupFile;
	}

	// Create the column header
	if (col->funcs->createHeader(&(col->columnHeader), columnName, err)) {
		goto cleanupFile;
	}

	// Read in the column header
	if (col->funcs->readInHeader(col->columnHeader, col->fp, err)) {
		goto cleanupHeader;
	}

	return 0;

cleanupHeader:
	col->funcs->destroyHeader(col->columnHeader);
cleanupFile:
	fclose(col->fp);
exit:
	return 1;
}

int columnWriteToDisk(column *col, error *err) {

	// Seek to the beginning of the file
	if (fseek(col->fp, 0, SEEK_SET) == -1) {
		err->err = ERR_INTERNAL;
		err->message = "Failed to seek in column file";
		return 1;
	}

	// Write the storage type to disk first
	if (fwrite(&(col->storageType), sizeof(COL_STORAGE_TYPE), 1, col->fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write storage type to column file";
		return 1;
	}

	// Next write header to disk
	if (col->funcs->writeOutHeader(col->columnHeader, col->fp, err)) {
		return 1;
	}

	if (fflush(col->fp)) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to flush column file";
		return 1;
	}

	return 0;
}

void columnPrint(column * col) {
	printf("Column:\n");
	printf("Storage type: %d\n", col->storageType);
	col->funcs->printHeader(col->columnHeader);
}



int columnInsert(column *col, int data, error *err) {
	if (col->funcs->insert(col->columnHeader, col->fp, data, err)) {
		return 1;
	}

	if (col->funcs->writeOutHeader(col->columnHeader, col->fp, err)) {
		return 1;
	}

	if (fflush(col->fp)) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to flush column file";
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