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
#include "dberror.h"
#include "command.h"
#include "output.h"
#include "filesys.h"
#include "debug.h"

#define READ_BLOCK_SIZE			256


int executeCommand(dbTableInfo *tbl, command *cmd, error *err) {
	printf("Received command: '%s' with args: '%s'\n", CMD_NAMES[cmd->cmd], cmd->args);
	int result = 0;

	// Check that table is in use if needed
	if (!tbl->isValid &&//*tableData == NULL &&
		
		(cmd->cmd != CMD_USE &&
		 cmd->cmd != CMD_CREATE_TABLE &&
		 cmd->cmd != CMD_REMOVE_TABLE)

		) {

		err->err = ERR_INVALID_CMD;
		err->message = "No table in use. Cannot execute command";
		return 1;
	}

	switch (cmd->cmd) {
		case CMD_USE:
			result = useTable(tbl, cmd->args, err);
			break;
		case CMD_CREATE_TABLE:
			result = createTable(cmd->args, err);
			break;
		case CMD_REMOVE_TABLE:
			result = removeTable(cmd->args, err);
			break;
		case CMD_CREATE:
			result = createColumn(tbl, cmd->args, err);
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
	dbTableInfo tempTbl;
	tempTbl.isValid = 1;
	strcpy(tempTbl.name, tableName);
	tempTbl.numColumns = 0;

	// Write table info to beginning of file
	if (fwrite(&tempTbl, sizeof(dbTableInfo), 1, fp) <= 0) {
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

int useTable(dbTableInfo *tbl, char *tableName, error *err) {
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
	if (fread(tbl, sizeof(dbTableInfo), 1, fp) < 1) {
		fclose(fp);

		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read table info from table";
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Using table '%s'\n", tbl->name);
	printdbTableInfo(tbl);
	return 0;
}

int createColumn(dbTableInfo *tbl, char *columnName, error * err) {
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
	dbColumnInfo tempCol;
	tempCol.size_bytes = 0;
	tempCol.storage_type = ST_INT;
	strcpy(tempCol.name, columnName);

	// Write table info to beginning of file
	if (fwrite(&tempCol, sizeof(dbColumnInfo), 1, fp) <= 0) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write to column file";
		fclose(fp);
		return 1;
	}

	// Cleanup
	fclose(fp);

	printf("Created new column '%s'\n", columnName);
	printdbColumnInfo(&tempCol);
	return 0;	
}

// int old_useTable(dbTableInfo *tbl, dbData *tableData, char *tableName, error *err) {

// 	printf("Size of table info: %d\n", (int) sizeof(dbTableInfo));

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
// 	if (fread(tbl, sizeof(dbTableInfo), 1, fp) < 1) {
// 		fclose(fp);

// 		err->err = ERR_MLFM_DATA;
// 		err->message = "Unable to read table info from table";
// 		return 1;
// 	}

// 	int bytesToRead = st.st_size - sizeof(dbTableInfo);
// 	printf("About to read %d bytes into memory from file %s\n", bytesToRead, tableName);

// 	// Read file into memory
// 	*tableData = (dbData) malloc(st.st_size - sizeof(dbTableInfo));
// 	if (fread(*tableData, st.st_size - sizeof(dbTableInfo), 1, fp) < 1) {
// 		err->err = ERR_MLFM_DATA;
// 		err->message = "Unable to read data from table";
// 		free(tableData);
// 		*tableData = NULL;
// 		return 1;
// 	}

// 	fclose(fp);
// 	return 0;
// }