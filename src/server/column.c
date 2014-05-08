#include <column.h>
#include <error.h>
#include <filesys.h>
#include <columnTypes/common.h>
#include <columnTypes/unsorted.h>
#include <columnTypes/sorted.h>
#include <columnTypes/btree/btree.h>

static int columnSetFunctions(column *col, error *err) {
	switch (col->storageType) {
		case COL_UNSORTED:
			col->funcs = &unsortedColumnFunctions;
			break;
		case COL_SORTED:
			col->funcs = &sortedColumnFunctions;
			break;
		case COL_BTREE:
			col->funcs = &btreeColumnFunctions;
			break;
		default:
			ERROR(err, E_COLST);
			return 1;
	}

	return 0;
}

// Allocates a new column struct and initializes it
int columnCreate(char *columnName, char *pathToDir, COL_STORAGE_TYPE storageType, FILE *headerFp, FILE *dataFp, column **col, error *err) {

	*col = (column *) malloc(sizeof(column));
	if (*col == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	(*col)->storageType = storageType;
	(*col)->headerFp = headerFp;
	(*col)->dataFp = dataFp;

	if (columnSetFunctions(*col, err)) {
		goto cleanupColumn;
	}

	if ((*col)->funcs->createHeader(&((*col)->columnHeader), columnName, pathToDir, err)) {
		goto cleanupColumn;
	}

	return 0;

cleanupColumn:
	free(*col);
exit:
	return 1;
}

void columnWipe(column *col) {
	col->storageType = -1;
	fclose(col->headerFp);
	fclose(col->dataFp);
	col->funcs->destroyHeader(col->columnHeader);
	col->funcs = NULL;
}

void columnDestroy(column *col) {
	columnWipe(col);
	free(col);
}

// Requires an allocated column as argument
int columnReadFromDisk(tableInfo *tbl, char *columnName, column *col, error *err) {
	char *tableName = tbl->name;

	char currentPath[BUFSIZE];


	// Open the column's data file
	columnPathData(currentPath, tableName, columnName);
	if (!fileExists(currentPath)) {
		ERROR(err, E_NOCOL);
		goto exit;
	}

	col->dataFp = fopen(currentPath, "rb+");
	if (col->dataFp == NULL) {
		ERROR(err, E_FOP);
		goto exit;
	}


	// Open the column's header file
	columnPathHeader(currentPath, tableName, columnName);
	if (!fileExists(currentPath)) {
		ERROR(err, E_NOCOL);
		goto cleanupDataFile;
	}

	col->headerFp = fopen(currentPath, "rb+");
	if (col->headerFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupDataFile;
	}



	// Read in the storage type
	if (fread(&(col->storageType), sizeof(COL_STORAGE_TYPE), 1, col->headerFp) < 1) {
		ERROR(err, E_FRD);
		goto cleanupHeaderFile;
	}

	// Setup column functions
	if (columnSetFunctions(col, err)) {
		goto cleanupHeaderFile;
	}

	// Read in the column header
	if (col->funcs->readInHeader(&(col->columnHeader), col->headerFp, err)) {
		goto cleanupHeaderFile;
	}

	return 0;

cleanupHeaderFile:
	fclose(col->headerFp);
cleanupDataFile:
	fclose(col->dataFp);
exit:
	return 1;
}

int columnWriteHeaderToDisk(column *col, error *err) {

	// Seek to the beginning of the file
	if (fseek(col->headerFp, 0, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	// Write the storage type to disk first
	if (fwrite(&(col->storageType), sizeof(COL_STORAGE_TYPE), 1, col->headerFp) < 1) {
		ERROR(err, E_FWR);
		return 1;
	}

	// Next write header to disk
	if (col->funcs->writeOutHeader(col->columnHeader, col->headerFp, err)) {
		return 1;
	}

	if (fflush(col->headerFp)) {
		ERROR(err, E_FFL);
		return 1;
	}

	return 0;
}

void columnPrint(column * col, char *message) {
	printf(">============ Print Column: %s\n", message);
	printf("Storage type: %d\n", col->storageType);
	printf(">------ Header:\n");
	col->funcs->printHeader(col->columnHeader);
	printf(">------ Data:\n");
	col->funcs->printData(col->columnHeader, col->dataFp);
	printf("=============\n");
}



int columnInsert(column *col, int data, error *err) {
	if (col->funcs->insert(col->columnHeader, col->dataFp, data, err)) {
		return 1;
	}

	if (col->funcs->writeOutHeader(col->columnHeader, col->headerFp, err)) {
		return 1;
	}

	if (fflush(col->headerFp) || fflush(col->dataFp)) {
		ERROR(err, E_FFL);
		return 1;
	}

	return 0;
}

int columnSelectAll(column *col, struct bitmap **bmp, error *err) {
	return col->funcs->selectAll(col->columnHeader, col->dataFp, bmp, err);
}

int columnSelectValue(column *col, int value, struct bitmap **bmp, error *err) {
	return col->funcs->selectValue(col->columnHeader, col->dataFp, value, bmp, err);
}
int columnSelectRange(column *col, int low, int high, struct bitmap **bmp, error *err) {
	return col->funcs->selectRange(col->columnHeader, col->dataFp, low, high, bmp, err);
}
int columnFetch(column *col, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err) {
	return col->funcs->fetch(col->columnHeader, col->dataFp, bmp, resultBytes, results, indices, err);
}
int columnLoad(column *col, int dataBytes, int *data, error *err) {
	if (col->funcs->load(col->columnHeader, col->dataFp, dataBytes, data, err)) {
		return 1;
	}

	if (col->funcs->writeOutHeader(col->columnHeader, col->headerFp, err)) {
		return 1;
	}

	if (fflush(col->headerFp) || fflush(col->dataFp)) {
		ERROR(err, E_FFL);
		return 1;
	}

	return 0;
}