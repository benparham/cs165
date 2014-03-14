#include <stdio.h>
#include <string.h>

#include <column.h>
#include <filesys.h>

// int getColDataSize(COL_DATA_TYPE type) {
// 	switch (type) {
// 		case COL_INT:
// 			return 4;
// 		default:
// 			return -1;
// 	}
// }

int strToColStorage(char *str, COL_STORAGE_TYPE *type) {
	int result = 1;

	if (strcmp(str, "unsorted") == 0) {
		*type = COL_UNSORTED;
		result = 0;
	} else if (strcmp(str, "sorted") == 0) {
		*type = COL_SORTED;
		result = 0;
	} else if (strcmp(str, "btree") == 0) {
		*type = COL_BTREE;
		result = 0;
	}

	return result;
}

void printColumnInfo(columnInfo *col) {
	printf("\nColumn:\n");

	printf("Name: %s\n", col->name);
	printf("Size: %d\n", col->sizeBytes);

	printf("Storage type: %d\n", col->storageType);
	// printf("Data type: %d\n", col->dataType);
}

int columnBufCreate(columnBuf **colBuf) {
	*colBuf = (columnBuf *) malloc(sizeof(columnBuf));
	if (*colBuf == NULL) {
		return 1;
	}

	return 0;
}

void columnBufDestroy(columnBuf *colBuf) {
	if (colBuf != NULL) {
		free(colBuf);
	}
}

int getColumnFromDisk(tableInfo *tbl, char *columnName, char *mode, columnBuf *colBuf, error *err) {

	if (colBuf == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Invalid column buffer";
		goto exit;
	}

	// Check that file exists
	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	if (!fileExists(pathToColumn)) {
		err->err = ERR_SRCH;
		err->message = "Cannot fetch column. Does not exist";
		goto exit;
	}

	// Open column file using 'mode'
	colBuf->fp = fopen(pathToColumn, mode);
	if (colBuf->fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file for column";
		goto exit;
	}

	// Read the column info from the beginning
	if (fread(&(colBuf->colInfo), sizeof(columnInfo), 1, colBuf->fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read column info from file";
		goto cleanupFile;
	}

	return 0;

cleanupFile:
	fclose(colBuf->fp);
exit:
	return 1;
}