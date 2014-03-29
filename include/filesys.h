#ifndef _FILESYS_H_
#define _FILESYS_H_

#include <string.h>

#include <error.h>

#define TBL_DIR					"tables"
#define COL_DIR					"columns"
#define DATA_DIR				"data"

#define TBL_HDR_FL_NM			"tableHeader.bin"
#define COL_HDR_FL_NM			"columnHeader.bin"
#define DATA_FL_NM				"data.bin"

#define DB_PTH					"./db"

typedef int fileOffset_t;

void tablePath(char *strPtr, char *tableName);
void tablePathHeader(char *strPtr, char *tableName);
void tablePathColumns(char *strPtr, char *tableName);

void columnPath(char *strPtr, char *tableName, char *columnName);
void columnPathHeader(char *strPtr, char *tableName, char *columnName);
void columnPathData(char *strPtr, char *tableName, char *columnName);

int fileExists(char *pathToFile);
int dirExists(char *pathToDir);
int removeDir(char *pathToDir, error *err);

#endif