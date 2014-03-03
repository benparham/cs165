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

#include "database.h"
#include "error.h"
#include "command.h"
#include "filesys.h"
#include "table.h"
#include "column.h"
#include "data.h"

int cmdNeedsTable(command *cmd) {

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
			result = useTable(tbl, (char *) cmd->args, err);
			break;
		case CMD_CREATE_TABLE:
			result = createTable((char *) cmd->args, err);
			break;
		case CMD_REMOVE_TABLE:
			// char *columnName = cmd->args;
			result = removeTable((char *) cmd->args, err);
			break;
		case CMD_CREATE:
			result = createColumn(tbl, (createColArgs *) cmd->args, err);
			break;
		case CMD_INSERT:
			result = insert(tbl, (insertArgs *) cmd->args, err);
			break;
		case CMD_SELECT:
			err->err = ERR_INTERNAL;
			err->message = "Select not yet implemented";
			result = 1;
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

int createTable(char *tableName, error *err) {

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

int removeTable(char *tableName, error *err) {
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

int useTable(tableInfo *tbl, char *tableName, error *err) {
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

int createColumn(tableInfo *tbl, createColArgs *args, error * err) {
	char *columnName = args->columnName;
	COL_DATA_TYPE dataType = args->dataType;
	COL_STORAGE_TYPE storageType = args->storageType;

	char pathToColumn[BUFSIZE];
	sprintf(pathToColumn, "%s/%s/%s/%s/%s.bin", DATA_PATH, TABLE_DIR, tbl->name, COLUMN_DIR, columnName);

	printf("Attempting to create column file %s\n", pathToColumn);

	if (fileExists(pathToColumn)) {
		err->err = ERR_DUP;
		err->message = "Cannot create column. Alread exists";
		return 1;
	}

	// Open the new column info file for writing
	FILE *fp = fopen(pathToColumn, "wb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create file for column";
		return 1;
	}

	// Initialize new table info with proper values
	columnInfo tempCol;
	tempCol.sizeBytes = 0;
	tempCol.storageType = storageType;
	tempCol.dataType = dataType;
	strcpy(tempCol.name, columnName);

	// Write table info to beginning of file
	if (fwrite(&tempCol, sizeof(columnInfo), 1, fp) <= 0) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write to column file";
		fclose(fp);
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Created new column '%s'\n", columnName);
	printColumnInfo(&tempCol);
	return 0;	
}

int sorted_insert(columnBuf *colBuf, error *err) {


	return 0;
}

int insert(tableInfo *tbl, insertArgs *args, error *err) {
	char *columnName = args->columnName;
	printf("Fetching column '%s'...\n", columnName);

	columnBuf *colBuf = fetchCol(tbl, columnName, err);
	if (colBuf == NULL) {
		printf("Col Buf was null\n");
		return 1;
	}

	int result = 0;

	printColumnInfo(&(colBuf->colInfo));

	switch (colBuf->colInfo.storageType) {
		case COL_UNSORTED:
			result = sorted_insert(colBuf, err);
			break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Column sort type unsupported";
			result = 1;
			break;
	}


	pthread_mutex_unlock(&(colBuf->colLock));

	return result;
}

// int old_useTable(tableInfo *tbl, dbData *tableData, char *tableName, error *err) {

// 	printf("Size of table info: %d\n", (int) sizeof(tableInfo));

// 	char pathToTable[BUFSIZE];
// 	getPathToTable(tableName, pathToTable);

// 	struct stat st;

// 	if (stat(pathToTable, &st) != 0) {
// 		err->err = ERR_SRCH;
// 		err->message = "Cannot use table. Does not exist";
// 		return 1;
// 	}

// 	// Open the table file
// 	FILE *fp = fopen(pathToTable, "rb+");
// 	if (fp == NULL) {
// 		err->err = ERR_INTERNAL;
// 		err->message = "Unable to open file";
// 		return 1;
// 	}

// 	// Read the table info from the beginning
// 	if (fread(tbl, sizeof(tableInfo), 1, fp) < 1) {
// 		fclose(fp);

// 		err->err = ERR_MLFM_DATA;
// 		err->message = "Unable to read table info from table";
// 		return 1;
// 	}

// 	int bytesToRead = st.st_size - sizeof(tableInfo);
// 	printf("About to read %d bytes into memory from file %s\n", bytesToRead, tableName);

// 	// Read file into memory
// 	*tableData = (dbData) malloc(st.st_size - sizeof(tableInfo));
// 	if (fread(*tableData, st.st_size - sizeof(tableInfo), 1, fp) < 1) {
// 		err->err = ERR_MLFM_DATA;
// 		err->message = "Unable to read data from table";
// 		free(tableData);
// 		*tableData = NULL;
// 		return 1;
// 	}

// 	fclose(fp);
// 	return 0;
// }