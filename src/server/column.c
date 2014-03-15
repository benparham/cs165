#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <column.h>
#include <global.h>
#include <filesys.h>
#include <columnTypes/common.h>
#include <columnTypes/unsorted.h>
#include <columnTypes/sorted.h>
#include <columnTypes/btree.h>
#include <bitmap.h>


int columnHeaderByteSize(COL_STORAGE_TYPE storageType) {
	switch (storageType) {
		case COL_UNSORTED:
			return sizeof(columnHeaderUnsorted);
		case COL_SORTED:
			return sizeof(columnHeaderSorted);
		case COL_BTREE:
			return sizeof(columnHeaderBtree);
		default:
			assert(0);
	}
}

void columnPrintHeader(COL_STORAGE_TYPE storageType, void *header) {
	printf("Header:\n");
	switch (storageType) {
		case COL_UNSORTED:
			unsortedPrintHeader(header);
			break;
		case COL_SORTED:
			sortedPrintHeader(header);
			break;
		case COL_BTREE:
			btreePrintHeader(header);
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
			if (unsortedCreateHeader(*header, columnName, err)) {
				goto deallocateHeader;
			}

			*sizeBytes = sizeof(columnHeaderUnsorted);
			break;
		case COL_SORTED:
			if ((*header = (columnHeaderSorted *) malloc(sizeof(columnHeaderSorted))) == NULL) {
				err->err = ERR_MEM;
				err->message = "Unable to allocate new column header";
				goto exit;
			}
			if (sortedCreateHeader(*header, columnName, err)) {
				goto deallocateHeader;
			}

			*sizeBytes = sizeof(columnHeaderSorted);
			break;
		case COL_BTREE:
			if ((*header = (columnHeaderBtree *) malloc(sizeof(columnHeaderBtree))) == NULL) {
				err->err = ERR_MEM;
				err->message = "Unable to allocate new column header";
				goto exit;
			}
			if (btreeCreateHeader(*header, columnName, err)) {
				goto deallocateHeader;
			}

			*sizeBytes = sizeof(columnHeaderBtree);
			break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Unsupported storage type for column";
			goto exit;
	}

	return 0;

deallocateHeader:
	free(*header);
exit:
	return 1;
}

void columnDestroyHeader(COL_STORAGE_TYPE storageType, void *header) {
	switch (storageType) {
		case COL_UNSORTED:
			unsortedDestroyHeader(header);
			break;
		case COL_SORTED:
			sortedDestroyHeader(header);
			break;
		case COL_BTREE:
			btreeDestroyHeader(header);
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
			col->insert = &unsortedInsert;
			col->selectAll = &unsortedSelectAll;
			col->selectValue = &unsortedSelectValue;
			col->selectRange = &unsortedSelectRange;
			col->fetch = &unsortedFetch;
			break;
		case COL_SORTED:
			columnHeaderSizeBytes = sizeof(columnHeaderSorted);
			col->insert = &sortedInsert;
			col->selectAll = &sortedSelectAll;
			col->selectValue = &sortedSelectValue;
			col->selectRange = &sortedSelectRange;
			col->fetch = &sortedFetch;
			break;
		case COL_BTREE:
			columnHeaderSizeBytes = sizeof(columnHeaderBtree);
			col->insert = &btreeInsert;
			col->selectAll = &btreeSelectAll;
			col->selectValue = &btreeSelectValue;
			col->selectRange = &btreeSelectRange;
			col->fetch = &btreeFetch;
			break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Unsupported column storage type";
			goto cleanupFile;
	}

	// Allocate space for the column header
	col->columnHeader = malloc(columnHeaderSizeBytes);

	// Read in the column header
	if (fread(col->columnHeader, columnHeaderSizeBytes, 1, col->fp) < 1) {
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


static int columnOverwriteHeader(column *col, error *err) {
	// Write data to end of file
	if (fseek(col->fp, sizeof(COL_STORAGE_TYPE), SEEK_SET) == -1) {
		err->err = ERR_INTERNAL;
		err->message = "Failed to seek in column file";
		return 1;
	}
	if (fwrite(col->columnHeader, columnHeaderByteSize(col->storageType), 1, col->fp) < 1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to overwrite header to column file";
		return 1;
	}

	return 0;
}

int columnInsert(column *col, int data, error *err) {
	if (col->insert(col->columnHeader, col->fp, data, err)) {
		return 1;
	}

	if (columnOverwriteHeader(col, err)) {
		return 1;
	}

	return 0;
}

int columnSelectAll(column *col, struct bitmap **bmp, error *err) {
	return col->selectAll(col->columnHeader, col->fp, bmp, err);
}

int columnSelectValue(column *col, int value, struct bitmap **bmp, error *err) {
	return col->selectValue(col->columnHeader, col->fp, value, bmp, err);
}
int columnSelectRange(column *col, int low, int high, struct bitmap **bmp, error *err) {
	return col->selectRange(col->columnHeader, col->fp, low, high, bmp, err);
}
int columnFetch(column *col, struct bitmap *bmp, error *err) {
	return col->fetch(col->columnHeader, col->fp, bmp, err);
}