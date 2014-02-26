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
	printf("Validity: %d\n", tbl->isValid);
	printf("Number of columns: %d\n", tbl->numColumns);

	for (int i; i < tbl->numColumns; i++) {
		printf("Column %d: %s\n", i, tbl->columns[i].name);
		printf("\tbytes: %d\n", tbl->columns[i].size_bytes);
		printf("\toffset: %d\n", tbl->columns[i].start_offset);
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

void getPathToTable(char *tableName, char *dest) {
	// Get path for the file
	char pathToTable[BUFSIZE];
	sprintf(pathToTable, "%s/%s/%s.bin", DATA_PATH, TABLE_PATH, tableName);

	strcpy(dest, pathToTable);
}

int tableExists(char *pathToTable) {
	struct stat st;
	return stat(pathToTable, &st) == 0;
}

int createTable(char *tableName, error *err) {

	char pathToTable[BUFSIZE];
	getPathToTable(tableName, pathToTable);

	if (tableExists(pathToTable)) {
		err->err = ERR_DUP;
		err->message = "Cannot create table. Already exists";
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
	tempTbl.isValid = 1;
	strcpy(tempTbl.name, tableName);
	tempTbl.numColumns = 0;

	// Write table info to beginning of file
	fwrite(&tempTbl, sizeof(dbTableInfo), 1, fp);

	printf("Created new table '%s'\n", tableName);

	// Close file
	fclose(fp);
	return 0;
}

int removeTable(char *tableName, error *err) {

	char pathToTable[BUFSIZE];
	getPathToTable(tableName, pathToTable);

	if (!tableExists(pathToTable)) {
		err->err = ERR_SRCH;
		err->message = "Cannot delete table. Does not exist";
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

	char pathToTable[BUFSIZE];
	getPathToTable(tableName, pathToTable);

	if (!tableExists(pathToTable)) {
		err->err = ERR_SRCH;
		err->message = "Cannot use table. Does not exist";
		return 1;
	}

	// Open the table file
	FILE *fp = fopen(pathToTable, "rb+");
	if (fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to open file";
		return 1;
	}

	// Read the table info from the beginning
	if (fread(tbl, sizeof(dbTableInfo), 1, fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read table info from table";
		return 1;
	}

	printf("Read in table:\n");
	printdbTableInfo(tbl);	

	printf("Using table: %s\n", tbl->name);
	return 0;
}