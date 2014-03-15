#ifndef _COMMON_H_
#define _COMMON_H_

typedef enum {
	COL_UNSORTED,
	COL_SORTED,
	COL_BTREE
} COL_STORAGE_TYPE;

int strToColStorage(char *str, COL_STORAGE_TYPE *type);

#endif