#ifndef _VARMAP_H_
#define _VARMAP_H_

#include <global.h>
#include <bitmap.h>
#include <error.h>

typedef struct varMapNode {

	char varName[NAME_SIZE];

	// struct bitmap *bmp;
	void *payload;

	struct varMapNode *next;
} varMapNode;

int varMapBootstrap();
void varMapCleanup();
void varMapPrint(char *message);

int varMapAddVar(char *varName, void *payload /*struct bitmap *bmp*/, error *err);
int varMapGetVar(char *varName, void **payload /*struct bitmap **bmp*/, error *err);

#endif