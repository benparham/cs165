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
#include <command.h>
#include <filesys.h>
#include <table.h>
#include <column.h>
#include <bitmap.h>
#include <varmap.h>

static int dbCreateTable(char *tableName, error *err) {

	char pathToTableDir[BUFSIZE];
	sprintf(pathToTableDir, "%s/%s/%s", DATA_PATH, TABLE_DIR, tableName);

	if (dirExists(pathToTableDir)) {
		err->err = ERR_DUP;
		err->message = "Cannot create table. Already exists";
		return 1;
	}
	
	// Create directory for new table
	if (mkdir(pathToTableDir, S_IRWXU | S_IRWXG) == -1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create directory for table";
		return 1;
	}

	char pathToTableColumnDir[BUFSIZE];
	sprintf(pathToTableColumnDir, "%s/%s", pathToTableDir, COLUMN_DIR);

	// Create directory for new table's columns
	if (mkdir(pathToTableColumnDir, S_IRWXU | S_IRWXG) == -1) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create directory for columns";
		return 1;
	}

	char pathToTableFile[BUFSIZE];
	sprintf(pathToTableFile, "%s/%s.bin", pathToTableDir, tableName);

	// Open the new table info file for writing
	FILE *fp = fopen(pathToTableFile, "wb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create file for table";
		return 1;
	}

	// Initialize new table info with proper values
	tableInfo tempTbl;
	tempTbl.isValid = 1;
	strcpy(tempTbl.name, tableName);
	tempTbl.numColumns = 0;

	// Write table info to beginning of file
	if (fwrite(&tempTbl, sizeof(tableInfo), 1, fp) <= 0) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write to table info file";

		fclose(fp);
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Created new table '%s'\n", tableName);
	return 0;
}

static int dbRemoveTable(char *tableName, error *err) {
	printf("Attempting to remove table '%s'\n", tableName);

	char pathToTableDir[BUFSIZE];
	sprintf(pathToTableDir, "%s/%s/%s", DATA_PATH, TABLE_DIR, tableName);

	if (!dirExists(pathToTableDir)) {
		err->err = ERR_SRCH;
		err->message = "Cannot delete table. Does not exist";
		return 1;
	}

	if (removeDir(pathToTableDir, err)) {
		return 1;
	}

	printf("Removed table %s\n", tableName);
	return 0;
}

static int dbUseTable(tableInfo *tbl, char *tableName, error *err) {
	char pathToTableFile[BUFSIZE];
	sprintf(pathToTableFile, "%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tableName, tableName);

	if (!fileExists(pathToTableFile)) {
		err->err = ERR_SRCH;
		err->message = "Cannot use table. Does not exist";
		return 1;
	}

	// Open the table file
	FILE *fp = fopen(pathToTableFile, "rb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file";
		return 1;
	}

	// Read the table info from the beginning
	if (fread(tbl, sizeof(tableInfo), 1, fp) < 1) {
		fclose(fp);

		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read table info from table";
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Using table '%s'\n", tbl->name);
	printtableInfo(tbl);
	return 0;
}

static int dbCreateColumn(tableInfo *tbl, createColArgs *args, error *err) {
	
	// TODO: Add code that updates the table info and writes it to disk

	char *columnName = args->columnName;
	COL_STORAGE_TYPE storageType = args->storageType;

	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	printf("Attempting to create column file %s\n", pathToColumn);

	if (fileExists(pathToColumn)) {
		err->err = ERR_DUP;
		err->message = "Cannot create column. Alread exists";
		goto exit;
	}

	// Open the new column info file
	FILE *fp = fopen(pathToColumn, "wb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create file for column";
		goto exit;
	}

	column *col;
	if (columnCreate(columnName, storageType, fp, &col, err)) {
		goto cleanupFile;
	}

	columnPrint(col, "created new column in dbCreateColumn");

	if (columnWriteToDisk(col, err)) {
		goto cleanupColumn;
	}

	printf("Wrote new column to disk\n");

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

static int dbInsert(tableInfo *tbl, insertArgs *args, error *err) {
	
	char *columnName = args->columnName;
	printf("Inserting into column '%s'...\n", columnName);

	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		err->err = ERR_MEM;
		err->message = "Could not allocate a column buffer";
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
	columnDestroy(col);

	return 0;

cleanupColumn:
	columnDestroy(col);
exit:
	return 1;
}

static int dbSelect(tableInfo *tbl, selectArgs *args, error *err) {

	char *columnName = args->columnName;
	printf("Selecting from column '%s'...\n", columnName);

	char *varName = args->varName;
	if (varName == NULL || strcmp(varName, "") == 0) {
		err->err = ERR_INTERNAL;
		err->message = "Invalid variable name";
		goto exit;
	}

	// Retrieve the column from disk
	column *col = (column *) malloc(sizeof(column));
	if (col == NULL) {
		err->err = ERR_MEM;
		err->message = "Could not allocate a column buffer";
		goto exit;
	}

	if (columnReadFromDisk(tbl, columnName, col, err)) {
		free(col);
		goto exit;
	}

	columnPrint(col, "read column from disk in dbSelect");

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

	varMapPrint("updated var map in dbSelect");

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

int executeCommand(tableInfo *tbl, command *cmd, error *err) {
	printf("Received command: '%s'\n", CMD_NAMES[cmd->cmd]);
	int result = 0;

	// Check that table is in use if needed
	if (!tbl->isValid && cmdNeedsTable(cmd)) {
		err->err = ERR_INVALID_CMD;
		err->message = "No table in use. Cannot execute command";
		return 1;
	}

	switch (cmd->cmd) {
		case CMD_USE:
			result = dbUseTable(tbl, (char *) cmd->args, err);
			break;
		case CMD_CREATE_TABLE:
			result = dbCreateTable((char *) cmd->args, err);
			break;
		case CMD_REMOVE_TABLE:
			result = dbRemoveTable((char *) cmd->args, err);
			break;
		case CMD_CREATE:
			result = dbCreateColumn(tbl, (createColArgs *) cmd->args, err);
			break;
		case CMD_INSERT:
			result = dbInsert(tbl, (insertArgs *) cmd->args, err);
			break;
		case CMD_SELECT:
			result = dbSelect(tbl, (selectArgs *) cmd->args, err);
			break;
		case CMD_FETCH:
			err->err = ERR_INTERNAL;
			err->message = "Fetch not yet implemented";
			result = 1;
			break;
		case CMD_EXIT:
			err->err = ERR_CLIENT_EXIT;
			err->message = "Client has exited";
			result = 1;
			break;
		default:
			err->err = ERR_INVALID_CMD;
			err->message = "Invalid Command";
			result = 1;
			break;
	}

	return result;
}