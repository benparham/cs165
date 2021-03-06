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

	// columnPrint(col, "read column from disk in dbInsert");

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
	if (varMapAddVar(varName, VAR_BMP, resultBmp, err)) {
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
	char *oldVarName = args->oldVarName;
	char *newVarName = args->newVarName;

	if (columnName == NULL || oldVarName == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}
	
	printf("Fetching from column '%s'...\n", columnName);

	// Get bitmap from var name
	struct bitmap *bmp;
	VAR_TYPE type;
	if (varMapGetVar(oldVarName, &type, (void **) (&bmp), err)) {
		goto exit;
	}

	if (type != VAR_BMP) {
		ERROR(err, E_VARTYPE);
		goto exit;
	}

	printf("Got bitmap for variable '%s'\n", oldVarName);
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
	int *indices;
	if (columnFetch(col, bmp, &resultBytes, &results, &indices, err)) {
		goto cleanupColumn;
	}

	// Return data to user if the haven't given a variable to store it in
	if (newVarName[0] == '\0') {

		int nResults = resultBytes / sizeof(int);
		printf("Got %d results from fetch\n", nResults);
		printf("[");
		for (int i = 0; i < nResults; i++) {
			printf("%d,", results[i]);
		}
		printf("]\n");


		if (resultBytes == 0) {
			results = NULL;
		}
		RESPONSE(res, "Fetch results:", resultBytes, results);

		free(indices);
	}

	// Otherwise, store data in variable
	else {

		// Add variable-results pair to varmap
		fetchResults *fResults = (fetchResults *) malloc(sizeof(fetchResults));
		if (fResults == NULL) {
			ERROR(err, E_NOMEM);
			goto cleanupColumn;
		}

		fResults->nColumnEntries = bitmapSize(bmp);
		fResults->nResultEntries = resultBytes / sizeof(int);
		fResults->results = results;
		fResults->indices = indices;

		if (varMapAddVar(newVarName, VAR_RESULTS, fResults, err)) {
			free(results);
			free(indices);
			free(fResults);
			goto cleanupColumn;
		}

		printf("Added variable '%s'\n", newVarName);
		RESPONSE_SUCCESS(res);
	}

	// Cleanup
	columnDestroy(col);
	
	return 0;

// cleanupResults:
// 	free(results);
// 	free(indices);
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
	MY_ASSERT(dataBytes % rowBytes == 0);
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
			qsort(rows, numRows, sizeof(int *), loadComp);
			break;
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

			MY_ASSERT(curColumn->storageType == COL_UNSORTED);
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

static int dbPrintVar(char *varName, response *res, error *err) {

	if (varName == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	void *payload;
	VAR_TYPE type;
	if (varMapGetVar(varName, &type, &payload, err)) {
		goto exit;
	}

	int resultBytes;
	int *results;

	if (type == VAR_BMP) {
		struct bitmap *bmp = (struct bitmap *) payload;

		printf("Bitmap result: \n");
		bitmapPrint(bmp);

		int bmpSize = bitmapSize(bmp);

		resultBytes = bmpSize * sizeof(int);
		results = (int *) malloc(resultBytes);

		if (resultBytes == 0) {
			free(results);
			results = NULL;
		} else {
			for (int i = 0; i < bmpSize; i++) {
				results[i] = bitmapIsSet(bmp, i) ? 1 : 0;
			}
		}
	} else if (type == VAR_RESULTS) {
		fetchResults *fResults = (fetchResults *) payload;

		resultBytes = fResults->nResultEntries * sizeof(int);
		results = (int *) malloc(resultBytes);
		memcpy(results, fResults->results, resultBytes);


		if (resultBytes == 0) {
			free(results);
			results = NULL;
		}
	} else {
		ERROR(err, E_VARTYPE);
		goto exit;
	}

	RESPONSE(res, "Print var results:", resultBytes, results);

	return 0;

exit:
	return 1;
}

static int obtainResults(char *varName, fetchResults **fResults, error *err) {

	if (varName == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	void *payload;
	VAR_TYPE type;
	if (varMapGetVar(varName, &type, &payload, err)) {
		goto exit;
	}

	if (type != VAR_RESULTS) {
		ERROR(err, E_VARTYPE);
		goto exit;
	}

	*fResults = (fetchResults *) payload;

	return 0;

exit:
	return 1;
}

static int dbMinimum(char *varName, response *res, error *err) {
	
	fetchResults *fResults;
	if (obtainResults(varName, &fResults, err)) {
		goto exit;
	}

	int nResults = fResults->nResultEntries;

	if (nResults == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = sizeof(int);
	int *minimum = (int *) malloc(resultBytes);
	if (minimum == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	*minimum = fResults->results[0];
	for (int i = 1; i < nResults; i++) {
		if (fResults->results[i] < *minimum) {
			*minimum = fResults->results[i];
		}
	}

	RESPONSE(res, "Minimum result:", resultBytes, minimum);

	return 0;

exit:
	return 1;
}

static int dbMaximum(char * varName, response *res, error *err) {
	fetchResults *fResults;
	if (obtainResults(varName, &fResults, err)) {
		goto exit;
	}

	int nResults = fResults->nResultEntries;

	if (nResults == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = sizeof(int);
	int *maximum = (int *) malloc(resultBytes);
	if (maximum == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	*maximum = fResults->results[0];
	for (int i = 1; i < nResults; i++) {
		if (fResults->results[i] > *maximum) {
			*maximum = fResults->results[i];
		}
	}

	RESPONSE(res, "Maximum result:", resultBytes, maximum);

	return 0;

exit:
	return 1;
}

static int dbSum(char * varName, response *res, error *err) {
	fetchResults *fResults;
	if (obtainResults(varName, &fResults, err)) {
		goto exit;
	}

	int nResults = fResults->nResultEntries;

	if (nResults == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = sizeof(int);
	int *sum = (int *) malloc(resultBytes);
	if (sum == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	*sum = 0;
	for (int i = 0; i < nResults; i++) {
		*sum += fResults->results[i];
	}

	RESPONSE(res, "Maximum result:", resultBytes, sum);

	return 0;

exit:
	return 1;
}

static int dbAverage(char * varName, response *res, error *err) {
	fetchResults *fResults;
	if (obtainResults(varName, &fResults, err)) {
		goto exit;
	}

	int nResults = fResults->nResultEntries;

	if (nResults == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = sizeof(int);
	int *average = (int *) malloc(resultBytes);
	if (average == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	*average = 0;
	for (int i = 0; i < nResults; i++) {
		*average += fResults->results[i];
	}
	*average /= nResults;

	RESPONSE(res, "Maximum result:", resultBytes, average);

	return 0;

exit:
	return 1;
}

static int dbCount(char * varName, response *res, error *err) {
	fetchResults *fResults;
	if (obtainResults(varName, &fResults, err)) {
		goto exit;
	}

	int nResults = fResults->nResultEntries;

	if (nResults == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = sizeof(int);
	int *count = (int *) malloc(resultBytes);
	if (count == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	*count = nResults;

	RESPONSE(res, "Maximum result:", resultBytes, count);

	return 0;

exit:
	return 1;
}

static int dbAdd(mathArgs *args, response *res, error *err) {
	char *var1 = args->var1;
	char *var2 = args->var2;

	if (var1 == NULL || var2 == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	fetchResults *fResults1;
	fetchResults *fResults2;

	if (obtainResults(var1, &fResults1, err) || obtainResults(var2, &fResults2, err)) {
		goto exit;
	}

	int nResults1 = fResults1->nResultEntries;
	int nResults2 = fResults2->nResultEntries;

	if (nResults1 == 0 || nResults1 != nResults2) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = nResults1 * sizeof(int);
	int *results = (int *) malloc(resultBytes);
	if (results == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	for (int i = 0; i < nResults1; i++) {
		results[i] = fResults1->results[i] + fResults2->results[i];
	}

	RESPONSE(res, "Add results:", resultBytes, results);

	return 0;

exit:
	return 1;
}

static int dbSubtract(mathArgs *args, response *res, error *err) {
	char *var1 = args->var1;
	char *var2 = args->var2;

	if (var1 == NULL || var2 == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	fetchResults *fResults1;
	fetchResults *fResults2;

	if (obtainResults(var1, &fResults1, err) || obtainResults(var2, &fResults2, err)) {
		goto exit;
	}

	int nResults1 = fResults1->nResultEntries;
	int nResults2 = fResults2->nResultEntries;

	if (nResults1 == 0 || nResults1 != nResults2) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = nResults1 * sizeof(int);
	int *results = (int *) malloc(resultBytes);
	if (results == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	for (int i = 0; i < nResults1; i++) {
		results[i] = fResults1->results[i] - fResults2->results[i];
	}

	RESPONSE(res, "Add results:", resultBytes, results);

	return 0;

exit:
	return 1;
}

static int dbMultiply(mathArgs *args, response *res, error *err) {
	char *var1 = args->var1;
	char *var2 = args->var2;

	if (var1 == NULL || var2 == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	fetchResults *fResults1;
	fetchResults *fResults2;

	if (obtainResults(var1, &fResults1, err) || obtainResults(var2, &fResults2, err)) {
		goto exit;
	}

	int nResults1 = fResults1->nResultEntries;
	int nResults2 = fResults2->nResultEntries;

	if (nResults1 == 0 || nResults1 != nResults2) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = nResults1 * sizeof(int);
	int *results = (int *) malloc(resultBytes);
	if (results == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	for (int i = 0; i < nResults1; i++) {
		results[i] = fResults1->results[i] * fResults2->results[i];
	}

	RESPONSE(res, "Add results:", resultBytes, results);

	return 0;

exit:
	return 1;
}

static int dbDivide(mathArgs *args, response *res, error *err) {
	char *var1 = args->var1;
	char *var2 = args->var2;

	if (var1 == NULL || var2 == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	fetchResults *fResults1;
	fetchResults *fResults2;

	if (obtainResults(var1, &fResults1, err) || obtainResults(var2, &fResults2, err)) {
		goto exit;
	}

	int nResults1 = fResults1->nResultEntries;
	int nResults2 = fResults2->nResultEntries;

	if (nResults1 == 0 || nResults1 != nResults2) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	int resultBytes = nResults1 * sizeof(int);
	int *results = (int *) malloc(resultBytes);
	if (results == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	for (int i = 0; i < nResults1; i++) {
		results[i] = fResults1->results[i] / fResults2->results[i];
	}

	RESPONSE(res, "Add results:", resultBytes, results);

	return 0;

exit:
	return 1;
}

static int dbLoopJoin(joinArgs *args, response *res, error *err) {
	
	char *oldVar1 = args->oldVar1;
	char *oldVar2 = args->oldVar2;

	char *newVar1 = args->newVar1;
	char *newVar2 = args->newVar2;

	if (oldVar1 == NULL || oldVar2 == NULL ||
		newVar1 == NULL || newVar2 == NULL) {
		ERROR(err, E_BADARG);
		goto exit;
	}

	// Get the results from the old var names
	fetchResults *fResults1;
	fetchResults *fResults2;

	if (obtainResults(oldVar1, &fResults1, err) || obtainResults(oldVar2, &fResults2, err)) {
		goto exit;
	}

	int nResults1 = fResults1->nResultEntries;
	int nResults2 = fResults2->nResultEntries;

	if (nResults1 == 0 || nResults2 == 0) {
		ERROR(err, E_VAREMT);
		goto exit;
	}

	// Create bitmaps to store results of joins
	struct bitmap *bmp1;
	struct bitmap *bmp2;
	if (bitmapCreate(fResults1->nColumnEntries, &bmp1, err)) {
		goto exit;
	}
	if (bitmapCreate(fResults2->nColumnEntries, &bmp2, err)) {
		bitmapDestroy(bmp1);
		goto exit;
	}

	// Determine smaller and larger results
	fetchResults *smallerFResults;
	fetchResults *largerFResults;

	struct bitmap *smallerBmp;
	struct bitmap *largerBmp;

	char *smallerNewVar;
	char *largerNewVar;

	if (nResults1 < nResults2) {
		smallerFResults = fResults1;
		smallerBmp = bmp1;
		smallerNewVar = newVar1;

		largerFResults = fResults2;
		largerBmp = bmp2;
		largerNewVar = newVar2;
	} else {
		smallerFResults = fResults2;
		smallerBmp = bmp2;
		smallerNewVar = newVar2;

		largerFResults = fResults1;
		largerBmp = bmp1;
		largerNewVar = newVar1;
	}

	// Loop and fill bitmaps
	for (int i = 0; i < largerFResults->nResultEntries; i++) {
		for (int j = 0; j < smallerFResults->nResultEntries; j++) {
			if (largerFResults->results[i] == smallerFResults->results[j]) {
				if (bitmapMark(largerBmp, largerFResults->indices[i], err) ||
					bitmapMark(smallerBmp, smallerFResults->indices[j], err)) {
					goto cleanupBitmaps;
				}
			}
		}
	}

	// Add bitmaps to varmap using new var names
	if (varMapAddVar(smallerNewVar, VAR_BMP, smallerBmp, err) ||
		varMapAddVar(largerNewVar, VAR_BMP, largerBmp, err)) {
		goto cleanupBitmaps;
	}

	printf("Added variables '%s' and '%s'\n", smallerNewVar, largerNewVar);

	RESPONSE_SUCCESS(res);

	return 0;

cleanupBitmaps:
	bitmapDestroy(bmp1);
	bitmapDestroy(bmp2);
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
		case CMD_PRINT_VAR:
			result = dbPrintVar((char *) cmd->args, res, err);
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
		case CMD_MIN:
			result = dbMinimum((char *) cmd->args, res, err);
			break;
		case CMD_MAX:
			result = dbMaximum((char *) cmd->args, res, err);
			break;
		case CMD_SUM:
			result = dbSum((char *) cmd->args, res, err);
			break;
		case CMD_AVG:
			result = dbAverage((char *) cmd->args, res, err);
			break;
		case CMD_CNT:
			result = dbCount((char *) cmd->args, res, err);
			break;
		case CMD_ADD:
			result = dbAdd((mathArgs *) cmd->args, res, err);
			break;
		case CMD_SUB:
			result = dbSubtract((mathArgs *) cmd->args, res, err);
			break;
		case CMD_MUL:
			result = dbMultiply((mathArgs *) cmd->args, res, err);
			break;
		case CMD_DIV:
			result = dbDivide((mathArgs *) cmd->args, res, err);
			break;
		case CMD_LOOPJOIN:
			result = dbLoopJoin((joinArgs *) cmd->args, res, err);
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