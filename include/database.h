#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdlib.h>

#include <error.h>
#include <response.h>
#include <command.h>
#include <table.h>
#include <connection.h>

int executeCommand(connection *con);

#endif