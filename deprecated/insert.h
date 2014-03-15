#ifndef _INSERT_H_
#define _INSERT_H_

#include <table.h>
#include <error.h>
#include <command.h>

int insert(tableInfo *tbl, insertArgs *args, error *err);

#endif