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

#include "database.h"
#include "dberror.h"
#include "command.h"
#include "output.h"

// Table functions
void printdbTableInfo(dbTableInfo *tbl) {
	printf("\nTable:\n");

	printf("Name: %s\n", tbl->name);
	printf("Number of rows: %d\n", tbl->numRows);

	for (int i; i < tbl->numColumns; i++) {
		printf("Column %d: %s\n", i, tbl->columns[i]);
	}

	printf("\n");
}

int executeCommand(dbTableInfo *tbl, command *cmd, error *err) {
	printf("Received command: '%s' with args: '%s'\n", CMD_NAMES[cmd->cmd], cmd->args);
	int result = 0;

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

void copyTable(dbTableInfo *dest, dbTableInfo *src) {
	dest->isValid = src->isValid;
	dest->numRows = src->numRows;
	dest->numColumns = src->numColumns;

	strcpy(dest->name, src->name);

	int length = sizeof(src->columns);
	for (int i = 0; i < length; i++) {
		strcpy(dest->columns[i], src->columns[i]);
	}
}

int createTable(char *tableName, error *err) {

	// Get path for the file
	char pathToTable[BUFSIZE];
	sprintf(pathToTable, "%s/%s/%s.bin", DATA_PATH, TABLE_PATH, tableName);

	// Check that the file doesn't already exist
	struct stat st;
	if (stat(pathToTable, &st) == 0) {
		err->err = ERR_DUP;
		err->message = "Cannot create file. Already exists";
		return 1;
	}

	// Open the new file for writing
	FILE *fp = fopen(pathToTable, "wb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to create file for table";
		return 1;
	}

	// Initialize new table info with proper values
	dbTableInfo tempTbl;
	strcpy(tempTbl.name, tableName);
	tempTbl.numRows = 0;
	tempTbl.numColumns = 0;
	tempTbl.isValid = 1;

	// Write table info to beginning of file
	fwrite(&tempTbl, sizeof(dbTableInfo), 1, fp);

	printf("Created new table '%s'\n", tableName);

	// Close file
	fclose(fp);
	return 0;
}

int removeTable(char *tableName, error *err) {

	// Get path for the file
	char pathToTable[BUFSIZE];
	sprintf(pathToTable, "%s/%s/%s.bin", DATA_PATH, TABLE_PATH, tableName);

	// Check that the file doesn't already exist
	struct stat st;
	if (stat(pathToTable, &st) != 0) {
		err->err = ERR_SRCH;
		err->message = "Cannot delete file. Does not exist";
		return 1;
	}

	if (remove(pathToTable) != 0) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to remove file for table";
		return 1;
	}

	printf("Removed table %s\n", tableName);

	return 0;
}

int useTable(dbTableInfo *tbl, char *tableName, error *err) {
	printf("Sanity check: %s\n", tableName);

	char mstrTablePath[BUFSIZE];
	sprintf(mstrTablePath, "%s/%s", DATA_PATH, MSTR_TBL_NAME);

	FILE *fp = fopen(mstrTablePath, "rb");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open master table";
		return 1;
	}

	int foundTable = 0;

	dbTableInfo *tempTbl = (dbTableInfo *) malloc(sizeof(dbTableInfo));

	while (fread(tempTbl, sizeof(dbTableInfo), 1, fp) > 0) {
		printf("Read in table: %s\n", tempTbl->name);
		printf("Searching for table: %s\n", tableName);

		if (strcmp(tempTbl->name, tableName) == 0) {
			copyTable(tbl, tempTbl);
			foundTable = 1;
			break;
		}
	}

	// Cleanup
	free(tempTbl);
	fclose(fp);

	// Table not found
	if (!foundTable) {
		err->err = ERR_SRCH;
		err->message = "Could not find table";
		return 1;
	}

	// Table found
	printf("Using table: %s\n", tbl->name);
	return 0;
}