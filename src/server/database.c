#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include <database.h>
#include <error.h>
#include <response.h>
#include <command.h>
#include <filesys.h>
#include <table.h>
#include <column.h>
#include <bitmap.h>
#include <varmap.h>
#include <connection.h>

static int dbCreateTable(char *tableName, response *res, error *err) {
	char currentPath[BUFSIZE];


	tablePath(currentPath, tableName);

	if (dirExists(currentPath)) {
		ERROR(err, E_DUPTBL);
		return 1;
	}
	
	// Create directory for new table
	if (mkdir(currentPath, S_IRWXU | S_IRWXG) == -1) {
		ERROR(err, E_MKDIR);
		return 1;
	}


	tablePathColumns(currentPath, tableName);

	// Create directory for new table's columns
	if (mkdir(currentPath, S_IRWXU | S_IRWXG) == -1) {
		ERROR(err, E_MKDIR);
		return 1;
	}


	tablePathHeader(currentPath, tableName);

	// Open the new table info file for writing
	FILE *fp = fopen(currentPath, "wb");
	if (fp == NULL) {
		ERROR(err, E_FOP);
		return 1;
	}


	// Initialize new table info with proper values
	tableInfo tempTbl;
	tempTbl.isValid = 1;
	strcpy(tempTbl.name, tableName);
	tempTbl.numColumns = 0;

	// Write table info to beginning of file
	if (fwrite(&tempTbl, sizeof(tableInfo), 1, fp) <= 0) {
		ERROR(err, E_FWR);

		fclose(fp);
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Created new table '%s'\n", tableName);
	RESPONSE_SUCCESS(res);

	return 0;
}

static int dbRemoveTable(char *tableName, response *res, error *err) {
	printf("Attempting to remove table '%s'\n", tableName);

	char pathToTableDir[BUFSIZE];
	tablePath(pathToTableDir, tableName);
	// sprintf(pathToTableDir, "%s/%s/%s", DB_PTH, TBL_DIR, tableName);

	if (!dirExists(pathToTableDir)) {
		ERROR(err, E_NOTBL);
		return 1;
	}

	if (removeDir(pathToTableDir, err)) {
		return 1;
	}

	printf("Removed table %s\n", tableName);
	RESPONSE_SUCCESS(res);

	return 0;
}

static int dbUseTable(tableInfo *tbl, char *tableName, response *res, error *err) {
	// char pathToTableFile[BUFSIZE];
	// sprintf(pathToTableFile, "%s/%s/%s/%s.bin", DB_PTH, TBL_DIR, tableName, tableName);

	char pathToTableHeader[BUFSIZE];
	tablePathHeader(pathToTableHeader, tableName);

	if (!fileExists(pathToTableHeader)) {
		ERROR(err, E_NOTBL);
		return 1;
	}

	// Open the table file
	FILE *fp = fopen(pathToTableHeader, "rb");
	if (fp == NULL) {
		ERROR(err, E_FOP);
		return 1;
	}

	// Read the table info from the beginning
	if (fread(tbl, sizeof(tableInfo), 1, fp) < 1) {
		fclose(fp);

		ERROR(err, E_FRD);
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Using table '%s'\n", tbl->name);
	RESPONSE_SUCCESS(res);

	// printtableInfo(tbl);
	
	return 0;
}

static int dbCreateColumn(tableInfo *tbl, createColArgs *args, response *res, error *err) {
	
	// TODO: Add code that updates the table info and writes it to disk

	char *tableName = tbl->name;
	char *columnName = args->columnName;
	COL_STORAGE_TYPE storageType = args->storageType;


	char currentPath[BUFSIZE];

	columnPath(currentPath, tableName, columnName);

	if (dirExists(currentPath)) {
		ERROR(err, E_DUPCOL);
		goto exit;
	}
	
	// Create directory for new column
	if (mkdir(currentPath, S_IRWXU | S_IRWXG) == -1) {
		ERROR(err, E_MKDIR);
		goto exit;
	}


	columnPathData(currentPath, tableName, columnName);

	// Create the column's data file
	FILE *dataFp = fopen(currentPath, "wb");
	if (dataFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupDir;
	}


	columnPathHeader(currentPath, tableName, columnName);

	// Create the column's header file
	FILE *headerFp = fopen(currentPath, "wb");
	if (headerFp == NULL) {
		ERROR(err, E_FOP);
		goto cleanupDataFile;
	}


	columnPath(currentPath, tableName, columnName);

	column *col;
	if (columnCreate(columnName, currentPath, storageType, headerFp, dataFp, &col, err)) {
		goto cleanupHeaderFile;
	}

	if (columnWriteHeaderToDisk(col, err)) {
		goto cleanupColumn;
	}

	// columnPrint(col, "created new column in dbCreateColumn");

	printf("Wrote new column to disk\n");
	RESPONSE_SUCCESS(res);

	columnDestroy(col);

	return 0;

cleanupColumn:
	columnDestroy(col);
cleanupHeaderFile:
	fclose(headerFp);
cleanupDataFile:
	fclose(dataFp);
cleanupDir:
	columnPath(currentPath, tableName, columnName);
	if (removeDir(currentPath, err)) {
		goto exit;
	}
exit:
	return 1;
}

static int dbRemoveColumn(tableInfo *tbl, char *columnName, response *res, error *err) {

	char *tableName = tbl->name;
	
	char currentPath[BUFSIZE];

	columnPath(currentPath, tableName, columnName);

	if (!dirExists(currentPath)) {
		ERROR(err, E_NOCOL);
		goto exit;
	}

	if (removeDir(currentPath, err)) {
		goto exit;
	}

	printf("Removed column %s\n", columnName);
	RESPONSE_SUCCESS(res);

	return 0;

exit:
	return 1;
}

static int dbInsert(tableInfo *tbl, insertArgs *args, response *res, error *err) {
	
	char *columnName = args->columnName;
	printf("Inserting into column '%s'...\n", columnName);

	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	if (columnReadFromDisk(tbl, columnName, col, err)) {
		free(col);
		goto exit;
	}

	columnPrint(col, "read column from disk in dbInsert");

	if (columnInsert(col, args->value, err)) {
		goto cleanupColumn;
	}

	printf("Inserted into column '%s'\n", columnName);
	RESPONSE_SUCCESS(res);

	columnDestroy(col);

	return 0;

cleanupColumn:
	columnDestroy(col);
exit:
	return 1;
}

static int dbSelect(tableInfo *tbl, selectArgs *args, response *res, error *err) {

	char *columnName = args->columnName;
	printf("Selecting from column '%s'...\n", columnName);

	char *varName = args->varName;
	if (varName == NULL || strcmp(varName, "") == 0) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	// Retrieve the column from disk
	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	if (columnReadFromDisk(tbl, columnName, col, err)) {
		free(col);
		goto exit;
	}

	// columnPrint(col, "read column from disk in dbSelect");

	// Get result bitmap
	struct bitmap *resultBmp;
	if (args->hasCondition) {
		if (args->isRange) {
			if (columnSelectRange(col, args->low, args->high, &resultBmp, err)) {
				goto cleanupColumn;
			}
		} else {
			if (columnSelectValue(col, args->low, &resultBmp, err)) {
				goto cleanupColumn;
			}
		}
	} else {
		if (columnSelectAll(col, &resultBmp, err)) {
			goto cleanupColumn;
		}
	}

	// Add variable-bitmap pair to varmap
	if (varMapAddVar(varName, resultBmp, err)) {
		goto cleanupBitmap;
	}

	// varMapPrint("updated var map in dbSelect", err);
	printf("Added variable '%s'\n", varName);
	RESPONSE_SUCCESS(res);

	columnDestroy(col);
	return 0;

cleanupBitmap:
	bitmapDestroy(resultBmp);
cleanupColumn:
	columnDestroy(col);
exit:
	return 1;
}

static int dbFetch(tableInfo *tbl, fetchArgs *args, response *res, error *err) {
	
	char *columnName = args->columnName;
	char *varName = args->varName;

	if (columnName == NULL || varName == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}
	
	printf("Fetching from column '%s'...\n", columnName);

	// Get bitmap from var name
	struct bitmap *bmp;
	if (varMapGetVar(varName, &bmp)) {
		ERROR(err, E_NOVAR);
		goto exit;
	}

	printf("Got bitmap for variable '%s'\n", varName);
	bitmapPrint(bmp);


	// Retrieve the column from disk
	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	if (columnReadFromDisk(tbl, columnName, col, err)) {
		free(col);
		goto exit;
	}

	// Fetch the results
	int resultBytes;
	int *results;
	if (columnFetch(col, bmp, &resultBytes, &results, err)) {
		goto cleanupColumn;
	}

	int nResults = resultBytes / sizeof(int);
	printf("Got %d results from fetch\n", nResults);
	printf("[");
	for (int i = 0; i < nResults; i++) {
		printf("%d,", results[i]);
	}
	printf("]\n");
	
	// int *returnData = (int *) malloc(5 * sizeof(int));
	// for (int i = 0; i < 5; i++) {
	// 	returnData[i] = i + 1;
	// }

	if (resultBytes == 0) {
		results = NULL;
	}
	RESPONSE(res, "Fetch results:", resultBytes, results);
	
	return 0;

cleanupColumn:
	columnDestroy(col);
exit:
	return 1;
}

static int loadComp(const void *A, const void *B) {
	int *a = *(int **) A;
	int *b = *(int **) B;

	return (a[0] - b[0]);
}

static int dbLoad(tableInfo *tbl, loadArgs *args, int dataBytes, void *data, response *res, error *err) {

	int numColumns = args->numColumns;
	char **columnNames = args->columnNames;
	if (columnNames == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	// Calculate number of rows based on size of data
	int rowBytes = numColumns * sizeof(int);
	assert(dataBytes % rowBytes == 0);
	int numRows = dataBytes / rowBytes;

	printf("Num columns: %d\n", numColumns);
	for (int i = 0; i < numColumns; i++) {
		printf("Column %d: %s\n", i, columnNames[i]);
	}

	printf("Data bytes: %d\n", dataBytes);
	printf("Num rows: %d\n", numRows);

	// Organize data into rows
	int **rows = (int **) malloc(numRows * sizeof(int *));

	for (int i = 0; i < numRows; i++) {
		int offset = i * rowBytes;
		rows[i] = data + offset;
	}

	for (int i = 0; i < numRows; i++) {
		printf("Row %d: ", i + 1);

		for (int j = 0; j < numColumns; j++) {
			printf("%d ", rows[i][j]);
		}

		printf("\n");
	}

	// Retrieve first column from disk
	column *curColumn = (column *) malloc(sizeof(column));
	if (curColumn == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupRows;
	}

	if (columnReadFromDisk(tbl, columnNames[0], curColumn, err)) {
		free(curColumn);
		goto cleanupRows;
	}

	// Sort if necessary
	switch(curColumn->storageType) {
		case COL_UNSORTED:
			break;
		case COL_SORTED:
			qsort(rows, numRows, sizeof(int *), loadComp);
			break;
		case COL_BTREE:
			ERROR(err, E_UNIMP);
			goto cleanupColumn;
		default:
			ERROR(err, E_COLST);
			goto cleanupColumn;
	}

	printf("After sorting\n");
	for (int i = 0; i < numRows; i++) {
		printf("Row %d: ", i + 1);

		for (int j = 0; j < numColumns; j++) {
			printf("%d ", rows[i][j]);
		}

		printf("\n");
	}


	
	// Setup buffer for column data
	int columnBytes = numRows * sizeof(int);
	int *columnData = (int *) malloc(columnBytes);
	if (columnData == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupColumn;
	}

	// Load columns one at a time
	for (int i = 0; i < numColumns; i++) {
		if (i != 0) {
			columnWipe(curColumn);

			if (columnReadFromDisk(tbl, columnNames[i], curColumn, err)) {
				goto cleanupColumnData;
			}

			assert(curColumn->storageType == COL_UNSORTED);
		}

		for (int j = 0; j < numRows; j++) {
			columnData[j] = rows[j][i];
		}

		if (columnLoad(curColumn, columnBytes, columnData, err)) {
			goto cleanupColumnData;
		}
	}

	free(columnData);
	columnDestroy(curColumn);
	free(rows);

	RESPONSE_SUCCESS(res);
	return 0;

cleanupColumnData:
	free(columnData);
cleanupColumn:
	columnDestroy(curColumn);
cleanupRows:
	free(rows);
exit:
	return 1;
}

static int dbPrint(tableInfo *tbl, char *columnName, response *res, error *err) {

	if (columnName == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}
	
	printf("Printing column '%s'...\n", columnName);

	// Retrieve the column from disk
	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	if (columnReadFromDisk(tbl, columnName, col, err)) {
		free(col);
		goto exit;
	}

	// Print column
	columnPrint(col, "");

	columnDestroy(col);
	RESPONSE_SUCCESS(res);

	return 0;

exit:
	return 1;
}

static int cmdNeedsTable(command *cmd) {

	return (cmd->cmd != CMD_USE &&
			cmd->cmd != CMD_CREATE_TABLE &&
			cmd->cmd != CMD_REMOVE_TABLE &&
			cmd->cmd != CMD_EXIT);
}

int executeCommand(connection *con) {
	tableInfo *tbl = con->tbl;
	command *cmd = con->cmd;
	response *res = con->res;
	error *err = con->err;

	int dataBytes = con->dataBytes;
	void *data = con->data;

	printf("Received command: '%s'\n", CMD_NAMES[cmd->cmd]);
	int result = 0;

	// Check that table is in use if needed
	if (!tbl->isValid && cmdNeedsTable(cmd)) {
		ERROR(err, E_USETBL);
		return 1;
	}

	switch (cmd->cmd) {
		case CMD_USE:
			result = dbUseTable(tbl, (char *) cmd->args, res, err);
			break;
		case CMD_CREATE_TABLE:
			result = dbCreateTable((char *) cmd->args, res, err);
			break;
		case CMD_REMOVE_TABLE:
			result = dbRemoveTable((char *) cmd->args, res, err);
			break;
		case CMD_CREATE:
			result = dbCreateColumn(tbl, (createColArgs *) cmd->args, res, err);
			break;
		case CMD_REMOVE:
			result = dbRemoveColumn(tbl, (char *) cmd->args, res, err);
			break;
		case CMD_INSERT:
			result = dbInsert(tbl, (insertArgs *) cmd->args, res, err);
			break;
		case CMD_SELECT:
			result = dbSelect(tbl, (selectArgs *) cmd->args, res, err);
			break;
		case CMD_FETCH:
			result = dbFetch(tbl, (fetchArgs *) cmd->args, res, err);
			break;
		case CMD_LOAD:
			result = dbLoad(tbl, (loadArgs *) cmd->args, dataBytes, data, res, err);
			break;
		case CMD_PRINT:
			result = dbPrint(tbl, (char *) cmd->args, res, err);
			break;
		case CMD_EXIT:
			ERROR(err, E_EXIT);
			result = 1;
			break;
		default:
			ERROR(err, E_INTERN);
			result = 1;
			break;
	}

	return result;
}