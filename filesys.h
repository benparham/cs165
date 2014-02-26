#ifndef _FILESYS_H_
#define _FILESYS_H_

#include "dberror.h"

int fileExists(char *pathToFile);
int dirExists(char *pathToDir);
int removeDir(char *pathToDir, error *err);

#endif