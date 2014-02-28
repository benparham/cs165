#ifndef _TABLE_H_
#define _TABLE_H_

#include "global.h"

typedef struct tableInfo {
	int isValid;

	// TODO: make these array sizes dynamic
	char name[NAME_SIZE];
	int numColumns;
} tableInfo;

void printtableInfo(tableInfo *tbl);

#endif