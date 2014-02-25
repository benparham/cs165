#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include "dberror.h"
#include "command.h"

#define MAX_COLS			64

#define DATA_PATH			"./db"
#define MSTR_TBL_NAME		"tables.csv"
// #define MSTR_TBLS_PATH		"./db/tables.csv"

/*
 * Tables
 */

typedef struct table {
	char *name;
	char *columns[MAX_COLS];
	int numRows;
	int numColumns;
	// FILE *fp;
} dbTable;

// Table functions
void printDbTable(dbTable *tbl);
int executeCommand(dbTable *tbl, command *cmd, error *err);
int createTable(char * tableName, error *err);
int useTable(dbTable *tbl, char *tableName, error *err);


#endif