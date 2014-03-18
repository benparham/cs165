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

	char pathToTableDir[BUFSIZE];
	sprintf(pathToTableDir, "%s/%s/%s", DATA_PATH, TABLE_DIR, tableName);

	if (dirExists(pathToTableDir)) {
		ERROR(err, E_DUPTBL);
		return 1;
	}
	
	// Create directory for new table
	if (mkdir(pathToTableDir, S_IRWXU | S_IRWXG) == -1) {
		ERROR(err, E_MKDIR);
		return 1;
	}

	char pathToTableColumnDir[BUFSIZE];
	sprintf(pathToTableColumnDir, "%s/%s", pathToTableDir, COLUMN_DIR);

	// Create directory for new table's columns
	if (mkdir(pathToTableColumnDir, S_IRWXU | S_IRWXG) == -1) {
		ERROR(err, E_MKDIR);
		return 1;
	}

	char pathToTableFile[BUFSIZE];
	sprintf(pathToTableFile, "%s/%s.bin", pathToTableDir, tableName);

	// Open the new table info file for writing
	FILE *fp = fopen(pathToTableFile, "wb");
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
	sprintf(pathToTableDir, "%s/%s/%s", DATA_PATH, TABLE_DIR, tableName);

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
	char pathToTableFile[BUFSIZE];
	sprintf(pathToTableFile, "%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tableName, tableName);

	if (!fileExists(pathToTableFile)) {
		ERROR(err, E_NOTBL);
		return 1;
	}

	// Open the table file
	FILE *fp = fopen(pathToTableFile, "rb");
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

	char *columnName = args->columnName;
	COL_STORAGE_TYPE storageType = args->storageType;

	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	printf("Attempting to create column file %s\n", pathToColumn);

	if (fileExists(pathToColumn)) {
		ERROR(err, E_DUPCOL);
		goto exit;
	}

	// Open the new column info file
	FILE *fp = fopen(pathToColumn, "wb");
	if (fp == NULL) {
		ERROR(err, E_FOP);
		goto exit;
	}

	column *col;
	if (columnCreate(columnName, storageType, fp, &col, err)) {
		goto cleanupFile;
	}

	// columnPrint(col, "created new column in dbCreateColumn");

	if (columnWriteToDisk(col, err)) {
		goto cleanupColumn;
	}

	printf("Wrote new column to disk\n");
	RESPONSE_SUCCESS(res);

	columnDestroy(col);

	return 0;

cleanupColumn:
	columnDestroy(col);
	goto removeFile;
cleanupFile:
	fclose(fp);
removeFile:
	remove(pathToColumn);
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

static int cmdNeedsTable(command *cmd) {

	return (cmd->cmd != CMD_USE &&
			cmd->cmd != CMD_CREATE_TABLE &&
			cmd->cmd != CMD_REMOVE_TABLE &&
			cmd->cmd != CMD_EXIT);
}

int executeCommand(connection *con) {//tableInfo *tbl, command *cmd, response *res, error *err) {
	tableInfo *tbl = con->tbl;
	command *cmd = con->cmd;
	response *res = con->res;
	error *err = con->err;

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
		case CMD_INSERT:
			result = dbInsert(tbl, (insertArgs *) cmd->args, res, err);
			break;
		case CMD_SELECT:
			result = dbSelect(tbl, (selectArgs *) cmd->args, res, err);
			break;
		case CMD_FETCH:
			ERROR(err, E_UNIMP);
			result = 1;
			break;
		case CMD_EXIT:
			ERROR(err, E_EXIT);
			result = 1;
			break;
		default:
			ERROR(err, E_UNIMP);
			result = 1;
			break;
	}

	return result;
}