#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include "dberror.h"
#include "command.h"

// TODO: get rid of this crap, make two or three sizes (large, middle, small) in a globals file
#define MAX_NAME_SIZE		16
#define MAX_COLS			16

#define DATA_PATH			"./db"
#define TABLE_DIR			"tables"
#define COLUMN_DIR			"columns"
// #define MSTR_TBL_NAME		"tables.csv"

/*
 * Tables
 */

typedef enum {
	ST_INT
} STORAGE_TYPE;

typedef struct dbColumnInfo {
	char name[MAX_NAME_SIZE];
	int size_bytes;
	STORAGE_TYPE storage_type;
	// int start_offset;
} dbColumnInfo;

typedef struct dbTableInfo {
	int isValid;

	// TODO: make these array sizes dynamic
	char name[MAX_NAME_SIZE];
	int numColumns;

	// dbColumnInfo columns[MAX_COLS];
} dbTableInfo; 

// Table functions
void printdbTableInfo(dbTableInfo *tbl);
int executeCommand(dbTableInfo *tbl, command *cmd, error *err);
int createTable(char *tableName, error *err);
int removeTable(char *tableName, error *err);
int useTable(dbTableInfo *tbl, char *tableName, error *err);
int createColumn(dbTableInfo *tbl, char *columnName, error *err);

#endif