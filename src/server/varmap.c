#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <varmap.h>
#include <bitmap.h>
#include <error.h>

/* 
 * Number of variables that can be stored at any given time
 * Once size is exceeded, old variables are removed in FIFO order
 */
#define VAR_MAP_SIZE	16

varMapNode *gVarMapHead;
// varMapNode *gVarMapTail;
int nodeCount;

static varMapNode* varMapNodeCreate(char *varName, struct bitmap *bmp) {
	if (strlen(varName) >= NAME_SIZE) {
		return NULL;
	}

	varMapNode *node = (varMapNode *) malloc(sizeof(varMapNode));
	if (node == NULL) {
		return NULL;
	}

	strcpy(node->varName, varName);
	node->bmp = bmp;
	node->next = NULL;

	return node;
}

static void varMapNodeDestroy(varMapNode *node) {
	free(node->bmp);
	free(node);
}

static void varMapPop() {
	varMapNode *firstNode = gVarMapHead;
	gVarMapHead = firstNode->next;
	varMapNodeDestroy(firstNode);

	nodeCount -= 1;
}

static void varMapPush(varMapNode *slot, varMapNode *newNode) {
	if (slot == NULL) {
		slot = newNode;
		nodeCount += 1;

		if (nodeCount >= VAR_MAP_SIZE) {
			varMapPop();
		}

		assert(nodeCount < VAR_MAP_SIZE);

		return;
	}

	varMapPush(slot->next, newNode);
}

static varMapNode *varMapFind(varMapNode *node, char *varName) {
	if (node == NULL) {
		return NULL;
	}

	if (strcmp(node->varName, varName) == 0) {
		return node;
	} else {
		return varMapFind(node->next, varName);
	}
}

int varMapBootstrap() {
	gVarMapHead = NULL;
	// gVarMapTail = NULL;
	nodeCount = 0;

	// TODO: Delete this (old test code)
	// int nbits = 8;
	// printf("Creating bitmap\n");
	// struct bitmap *bmp = bitmapCreate(nbits);
	// printf("Bitmap size: %d\n", bitmapSize(bmp));
	// printf("First bit marked: %s\n", bitmapIsSet(bmp, 0) ? "true" : "false");
	// printf("Marking first bit\n");
	// bitmapMark(bmp, 0);
	// printf("First bit marked: %s\n", bitmapIsSet(bmp, 0) ? "true" : "false");
	// printf("Unmarking first bit\n");
	// bitmapUnmark(bmp, 0);
	// printf("First bit marked: %s\n", bitmapIsSet(bmp, 0) ? "true" : "false");

	return 0;
}

void varMapCleanup() {
	while (nodeCount > 0) {
		varMapPop();
	}
}

void varMapNodePrint(varMapNode *node) {
	printf("Variable: %s\n", node->varName);
}

static void _varMapPrint(varMapNode *node) {
	if (node != NULL) {
		varMapNodePrint(node);
		_varMapPrint(node->next);
	}
}

void varMapPrint() {
	_varMapPrint(gVarMapHead);
}

int varMapAddVar(char *varName, struct bitmap *bmp, error *err) {

	varMapNode *newNode = varMapNodeCreate(varName, bmp);
	if (newNode == NULL) {
		err->err = ERR_MEM;
		err->message = "Failed to create new variable node";
		return 1;
	}

	varMapPush(gVarMapHead, newNode);

	return 0;
}
int varMapGetVar(char *varName, struct bitmap **bmp) {

	varMapNode *node = varMapFind(gVarMapHead, varName);
	if (node == NULL) {
		return 1;
	}

	*bmp = node->bmp;

	return 0;
}