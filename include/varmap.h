#ifndef _VARMAP_H_
#define _VARMAP_H_

#include <global.h>
#include <bitmap.h>
#include <error.h>

// Possible payload types
typedef enum {
	VAR_BMP,
	VAR_RESULTS
} VAR_TYPE;

// typedef struct fetchResults {
// 	int sizeBytes;
// 	int *results;
// } fetchResults;

typedef struct fetchResults {
	int nColumnEntries;
	int nResultEntries;
	int *results;
	int *indices;
} fetchResults;

typedef struct varMapNode {

	VAR_TYPE type;

	char varName[NAME_SIZE];

	// struct bitmap *bmp;
	void *payload;

	struct varMapNode *next;
} varMapNode;

int varMapBootstrap();
void varMapCleanup();
void varMapPrint(char *message);

int varMapAddVar(char *varName, VAR_TYPE type, void *payload /*struct bitmap *bmp*/, error *err);
int varMapGetVar(char *varName, VAR_TYPE *type, void **payload /*struct bitmap **bmp*/, error *err);

#endif