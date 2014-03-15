#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include <error.h>
#include <global.h>
#include <command.h>
#include <table.h>
#include <column.h>

int executeCommand(tableInfo *tbl, command *cmd, error *err);

#endif