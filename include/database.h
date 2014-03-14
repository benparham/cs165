#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include "error.h"
#include "global.h"
#include "command.h"
#include "table.h"
#include "column.h"

int executeCommand(tableInfo *tbl, command *cmd, error *err);
int createTable(char *tableName, error *err);
int removeTable(char *tableName, error *err);
int useTable(tableInfo *tbl, char *tableName, error *err);
int createColumn(tableInfo *tbl, createColArgs *args, error *err);
int insert(tableInfo *tbl, insertArgs *args, error *err);

#endif