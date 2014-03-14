#ifndef _FILESYS_H_
#define _FILESYS_H_

#include "error.h"

#define DATA_PATH			"./db"
#define TABLE_DIR			"tables"
#define COLUMN_DIR			"columns"

int fileExists(char *pathToFile);
int dirExists(char *pathToDir);
int removeDir(char *pathToDir, error *err);

#endif