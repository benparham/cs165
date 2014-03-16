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

static int varMapPush(varMapNode **slot, char *varName, struct bitmap *bmp, error *err) {
	// If slot is empty
	if (*slot == NULL) {
		varMapNode *newNode = varMapNodeCreate(varName, bmp);
		if (newNode == NULL) {
			err->err = ERR_MEM;
			err->message = "Failed to create new variable node";
			return 1;
		}

		*slot = newNode;
		nodeCount += 1;

		if (nodeCount >= VAR_MAP_SIZE) {
			varMapPop();
		}

		assert(nodeCount < VAR_MAP_SIZE);

		return 0;
	}

	// If slot already contains varName
	if (strcmp((*slot)->varName, varName) == 0) {
		(*slot)->bmp = bmp;

		return 0;
	}

	// If slot is full
	return varMapPush(&((*slot)->next), varName, bmp, err);

}

static varMapNode* varMapFind(varMapNode *node, char *varName) {
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

	return 0;
}

void varMapCleanup() {
	while (nodeCount > 0) {
		varMapPop();
	}
}

void varMapNodePrint(varMapNode *node) {
	printf("Variable: %s\n", node->varName);
	printf("Bitmap:\n");
	bitmapPrint(node->bmp);
}

static void _varMapPrint(varMapNode *node) {
	if (node != NULL) {
		varMapNodePrint(node);
		_varMapPrint(node->next);
	}
}

void varMapPrint(char *message) {
	printf(">============ Print Var Map: %s\n", message);
	printf("Node count: %d\n", nodeCount);
	_varMapPrint(gVarMapHead);
	printf("=============\n");
}

int varMapAddVar(char *varName, struct bitmap *bmp, error *err) {
	return varMapPush(&gVarMapHead, varName, bmp, err);
}

int varMapGetVar(char *varName, struct bitmap **bmp) {

	varMapNode *node = varMapFind(gVarMapHead, varName);
	if (node == NULL) {
		return 1;
	}

	*bmp = node->bmp;

	return 0;
}