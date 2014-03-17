#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include <error.h>
#include <response.h>
#include <command.h>
#include <table.h>

int executeCommand(tableInfo *tbl, command *cmd, response *res, error *err);

#endif