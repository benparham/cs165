#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <column.h>
#include <global.h>
#include <filesys.h>

// ==================== Internal ======================


// Unsorted
typedef struct columnHeaderUnsorted {
	char name[NAME_SIZE];
	int sizeBytes;
} columnHeaderUnsorted;

void columnPrintHeaderUnsorted(columnHeaderUnsorted *header) {
	printf("Name: %s\nSize Bytes: %d\n", header->name, header->sizeBytes);
}

int columnCreateHeaderUnsorted(columnHeaderUnsorted *header, char *columnName, error *err) {
	// header->name = name;
	strcpy(header->name, columnName);
	header->sizeBytes = 0;
	return 0;
}

void columnDestroyHeaderUnsorted(columnHeaderUnsorted *header) {
	free(header);
}

int columnInsertUnsorted(void *columnHeader, FILE *fp, char *data, error *err) {

	err->err = ERR_UNIMP;
	err->message = "Insert unimplemented for sorted columns";
	return 1;
}

// Sorted

// Btree



// ==================== External ======================

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

void columnPrintHeader(COL_STORAGE_TYPE storageType, void *header) {
	printf("Header:\n");
	switch (storageType) {
		case COL_UNSORTED:
			columnPrintHeaderUnsorted(header);
			break;
		default:
			printf("Unknown storage type for column\n");
			break;
	}
}

void columnPrint(column *col) {
	printf("Storage Type: %d\n", col->storageType);
	columnPrintHeader(col->storageType, col->columnHeader);
}

// On success this will allocate header and return its size in bytes via sizeBytes
int columnCreateNewHeader(COL_STORAGE_TYPE storageType, char *columnName, void **header, int *sizeBytes, error *err) {
	switch (storageType) {
		case COL_UNSORTED:
			if ((*header = (columnHeaderUnsorted *) malloc(sizeof(columnHeaderUnsorted))) == NULL) {
				err->err = ERR_MEM;
				err->message = "Unable to allocate new column header";
				goto exit;
			}
			if (columnCreateHeaderUnsorted(*header, columnName, err)) {
				goto deallocateHeader;
			}

			*sizeBytes = sizeof(columnHeaderUnsorted);
			break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Unsupported storage type for column";
			goto exit;
	}

	return 0;

deallocateHeader:
	free(header);
exit:
	return 1;
}

void columnDestroyHeader(COL_STORAGE_TYPE storageType, void *header) {
	switch (storageType) {
		case COL_UNSORTED:
			columnDestroyHeaderUnsorted(header);
			break;
		default:
			return;
	}
}

// Requires an allocated column argument
int columnCreateFromDisk(tableInfo *tbl, char *columnName, column *col, error *err) {
	
	if (col == NULL) {
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

	// Open column file
	col->fp = fopen(pathToColumn, "rb+");
	if (col->fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file for column";
		goto exit;
	}

	// Read the column storage type
	if (fread(&(col->storageType), sizeof(COL_STORAGE_TYPE), 1, col->fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read column storage type from file";
		goto cleanupFile;
	}

	// Determine how large the column header is via the storage type
	// Also, set functions pointers to appropriate functions
	int columnHeaderSizeBytes;
	switch (col->storageType) {
		case COL_UNSORTED:
			columnHeaderSizeBytes = sizeof(columnHeaderUnsorted);
			col->insert = &columnInsertUnsorted;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Unsupported column storage type";
			goto cleanupFile;
	}

	// Allocate space for the column header
	col->columnHeader = malloc(columnHeaderSizeBytes);

	// Read in the column header
	if (fread(&(col->columnHeader), columnHeaderSizeBytes, 1, col->fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read column info from file";
		goto cleanupColumnHeader;
	}

	return 0;

cleanupColumnHeader:
	free(col->columnHeader);
cleanupFile:
	fclose(col->fp);
exit:
	return 1;
}

// Should only be called on a successfully created column, i.e. do not call
// after an error is returned from columnCreate*. In that case, just free
// the column itself if it was allocated
void columnDestroy(column *col) {
	assert(col != NULL);
	
	columnDestroyHeader(col->storageType, col->columnHeader);
	fclose(col->fp);
	free(col);
}

int columnInsert(column *col, char *data, error *err) {
	return col->insert(col->columnHeader, col->fp, data, err);
}


// void printColumnInfo(columnInfo *col) {
// 	printf("\nColumn:\n");

// 	printf("Name: %s\n", col->name);
// 	printf("Size: %d\n", col->sizeBytes);

// 	printf("Storage type: %d\n", col->storageType);
// }

// int columnBufCreate(columnBuf **colBuf) {
// 	*colBuf = (columnBuf *) malloc(sizeof(columnBuf));
// 	if (*colBuf == NULL) {
// 		return 1;
// 	}

// 	return 0;
// }

// void columnBufDestroy(columnBuf *colBuf) {
// 	if (colBuf != NULL) {
// 		free(colBuf);
// 	}
// }

// int getColumnFromDisk(tableInfo *tbl, char *columnName, char *mode, columnBuf *colBuf, error *err) {

// 	if (colBuf == NULL) {
// 		err->err = ERR_INTERNAL;
// 		err->message = "Invalid column buffer";
// 		goto exit;
// 	}

// 	// Check that file exists
// 	char pathToColumn[BUFSIZE];
// 	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

// 	if (!fileExists(pathToColumn)) {
// 		err->err = ERR_SRCH;
// 		err->message = "Cannot fetch column. Does not exist";
// 		goto exit;
// 	}

// 	// Open column file using 'mode'
// 	colBuf->fp = fopen(pathToColumn, mode);
// 	if (colBuf->fp == NULL) {
// 		err->err = ERR_INTERNAL;
// 		err->message = "Unable to open file for column";
// 		goto exit;
// 	}

// 	// Read the column info from the beginning
// 	if (fread(&(colBuf->colInfo), sizeof(columnInfo), 1, colBuf->fp) < 1) {
// 		err->err = ERR_MLFM_DATA;
// 		err->message = "Unable to read column info from file";
// 		goto cleanupFile;
// 	}

// 	return 0;

// cleanupFile:
// 	fclose(colBuf->fp);
// exit:
// 	return 1;
// }