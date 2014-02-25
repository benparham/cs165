#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include "dberror.h"
#include "command.h"

// TODO: get rid of this crap, make two or three sizes (large, middle, small) in a globals file
#define MAX_NAME_SIZE		64
#define MAX_COLS			64

#define DATA_PATH			"./db"
#define MSTR_TBL_NAME		"tables.csv"

/*
 * Tables
 */

typedef struct table {
	int isValid;

	// TODO: make these array sizes dynamic
	char name[MAX_NAME_SIZE];
	char columns[MAX_NAME_SIZE][MAX_NAME_SIZE];
	int numRows;
	int numColumns;
	// FILE *fp;
} dbTableInfo;

// Table functions
void printdbTableInfo(dbTableInfo *tbl);
int executeCommand(dbTableInfo *tbl, command *cmd, error *err);
int createTable(char * tableName, error *err);
int useTable(dbTableInfo *tbl, char *tableName, error *err);


#endif