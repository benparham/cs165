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
int nodeCount;

static varMapNode* varMapNodeCreate(char *varName, VAR_TYPE type, void *payload /*struct bitmap *bmp*/) {
	if (strlen(varName) >= NAME_SIZE) {
		return NULL;
	}

	varMapNode *node = (varMapNode *) malloc(sizeof(varMapNode));
	if (node == NULL) {
		return NULL;
	}

	strcpy(node->varName, varName);
	node->type = type;
	// node->bmp = bmp;
	node->payload = payload;
	node->next = NULL;

	return node;
}

static void varMapNodeDestroy(varMapNode *node) {
	// free(node->bmp);
	free(node->payload);
	free(node);
}

static void varMapPop() {
	varMapNode *firstNode = gVarMapHead;
	gVarMapHead = firstNode->next;
	varMapNodeDestroy(firstNode);

	nodeCount -= 1;
}

static int varMapPush(varMapNode **slot, char *varName, VAR_TYPE type, void *payload /*struct bitmap *bmp*/, error *err) {
	// If slot is empty
	if (*slot == NULL) {
		varMapNode *newNode = varMapNodeCreate(varName, type, payload /*bmp*/);
		if (newNode == NULL) {
			ERROR(err, E_VARND);
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
		// (*slot)->bmp = bmp;
		(*slot)->payload = payload;

		return 0;
	}

	// If slot is full
	return varMapPush(&((*slot)->next), varName, type, payload /*bmp*/, err);
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
	nodeCount = 0;

	return 0;
}

void varMapCleanup() {
	while (nodeCount > 0) {
		varMapPop();
	}
}

static void varMapNodePrint(varMapNode *node) {
	printf("Variable: %s\n", node->varName);
	// printf("Bitmap:\n");
	// bitmapPrint(node->bmp);
}

static void recVarMapPrint(varMapNode *node) {
	if (node != NULL) {
		varMapNodePrint(node);
		recVarMapPrint(node->next);
	}
}

void varMapPrint(char *message) {
	printf(">============ Print Var Map: %s\n", message);
	printf("Node count: %d\n", nodeCount);
	recVarMapPrint(gVarMapHead);
	printf("=============\n");
}

int varMapAddVar(char *varName, VAR_TYPE type, void *payload /*struct bitmap *bmp*/, error *err) {
	return varMapPush(&gVarMapHead, varName, type, payload /*bmp*/, err);
}

int varMapGetVar(char *varName, VAR_TYPE *type, void **payload /*struct bitmap **bmp*/, error *err) {

	varMapNode *node = varMapFind(gVarMapHead, varName);
	if (node == NULL) {
		return 1;
	}

	*type = node->type;
	// *bmp = node->bmp;
	*payload = node->payload;

	return 0;
}