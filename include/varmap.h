#ifndef _VARMAP_H_
#define _VARMAP_H_

#include <global.h>
#include <bitmap.h>
#include <error.h>

typedef struct varMapNode {

	char varName[NAME_SIZE];
	struct bitmap *bmp;

	struct varMapNode *next;
} varMapNode;

int varMapBootstrap();
void varMapCleanup();

int varMapAddVar(char *varName, struct bitmap *bmp, error *err);
int varMapGetVar(char *varName, struct bitmap **bmp);

#endif