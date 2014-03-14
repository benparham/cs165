#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include <error.h>
#include <global.h>
#include <command.h>
#include <table.h>
#include <column.h>

int executeCommand(tableInfo *tbl, command *cmd, error *err);

int dbCreateTable(char *tableName, error *err);
int dbRemoveTable(char *tableName, error *err);
int dbUseTable(tableInfo *tbl, char *tableName, error *err);
int dbCreateColumn(tableInfo *tbl, createColArgs *args, error *err);
int dbInsert(tableInfo *tbl, insertArgs *args, error *err);

#endif