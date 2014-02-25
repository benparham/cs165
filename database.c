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

// Table functions
void printDbTable(dbTable *tbl) {
	printf("\nTable:\n");

	printf("Name: %s\n", tbl->name);
	printf("Number of rows: %d\n", tbl->numRows);

	for (int i; i < tbl->numColumns; i++) {
		printf("Column %d: %s\n", i, tbl->columns[i]);
	}

	printf("\n");
}

int executeCommand(dbTable *tbl, command *cmd, error *err) {
	printf("Received command: '%s' with args: '%s'\n", CMD_NAMES[cmd->cmd], cmd->args);
	int result = 0;

	switch (cmd->cmd) {
		case CMD_USE:
			result = useTable(tbl, cmd->args, err);
			break;
		case CMD_CREATE_TABLE:
			result = createTable(cmd->args, err);
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

// TODO: efficiently check that table doesn't already exist
int createTable(char *tableName, error *err) {
	printf("Will create table with name '%s' here...\n", tableName);

	char mstrTablePath[BUFSIZE];
	sprintf(mstrTablePath, "%s/%s", DATA_PATH, MSTR_TBL_NAME);

	// printf("Master table path: %s\n", mstrTablePath);

	FILE *fp = fopen(mstrTablePath, 'ab');


	return 0;
}

int useTable(dbTable *tbl, char *tableName, error *err) {
	int foundTable = 0;


	FILE *mstr_fp;
	mstr_fp = fopen("./db/tables.csv", "r");
	if (mstr_fp == NULL) {
		err->err = ERR_INTERNAL;
		err->message = "Master tables file missing";
		return 1;
	}

	size_t len = 0;
	char *line = NULL;
	while (getline(&line, &len, mstr_fp) != -1) {
		if (strncmp(line, tableName, strlen(tableName)) == 0) {
			
			// Read in table name
			char *tok = strtok(line, ",");
			if (tok == NULL) {
				err->err = ERR_MLFM_DATA;
				err->message = "Table name missing";
				return 1;
			}
			tbl->name = tok;

			// Read in table size
			tok = strtok(NULL, ",");
			if (tok == NULL) {
				err->err = ERR_MLFM_DATA;
				err->message = "Table's number of rows missing";
				return 1;
			}
			tbl->numRows = atoi(tok);

			// Read in table columns
			tok = strtok(NULL, ",\n");
			if (tok == NULL) {
				err->err = ERR_MLFM_DATA;
				err->message = "Table has no columns";
				return 1;
			}
			tbl->numColumns = 0;
			while (tok != NULL) {
				tbl->columns[tbl->numColumns] = tok;
				tbl->numColumns++;
				tok = strtok(NULL, ",\n");
			}

			// Open table's file with read/write access
			// char filePath[BUFSIZE];
			// sprintf(filePath, "%s/%s", MSTR_TBLS_PATH, tableName);
			// printf("Table file to open: %s\n", filePath);
			// tbl->fp = fopen(, "r+");

			foundTable = 1;
			break;
		}
	}

	fclose(mstr_fp);
	
	if (!foundTable) {
		err->err = ERR_SRCH;
		err->message = "Could not find table";
		return 1;
	}

	printf("Using table: '%s'\n", tbl->name);
	return 0;
}