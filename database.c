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

#define READ_BLOCK_SIZE			256

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

int executeCommand(dbTableInfo *tbl, dbData *tableData, command *cmd, error *err) {
	printf("Received command: '%s' with args: '%s'\n", CMD_NAMES[cmd->cmd], cmd->args);
	int result = 0;

	// Check that table is in use if needed
	if (*tableData == NULL &&
		
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
			result = useTable(tbl, tableData, cmd->args, err);
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

int useTable(dbTableInfo *tbl, dbData *tableData, char *tableName, error *err) {

	printf("Size of table info: %d\n", (int) sizeof(dbTableInfo));

	char pathToTable[BUFSIZE];
	getPathToTable(tableName, pathToTable);

	struct stat st;

	if (stat(pathToTable, &st) != 0) {
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
		fclose(fp);

		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read table info from table";
		return 1;
	}

	int bytesToRead = st.st_size - sizeof(dbTableInfo);
	printf("About to read %d bytes into memory from file %s\n", bytesToRead, tableName);

	// Read file into memory
	*tableData = (dbData) malloc(st.st_size - sizeof(dbTableInfo));
	if (fread(*tableData, st.st_size - sizeof(dbTableInfo), 1, fp) < 1) {
		err->err = ERR_MLFM_DATA;
		err->message = "Unable to read data from table";
		free(tableData);
		*tableData = NULL;
		return 1;
	}

	fclose(fp);
	return 0;
}