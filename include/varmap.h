#ifndef _VARMAP_H_
#define _VARMAP_H_

typedef struct varMapNode {

	char *varName;
	// bitvector

	struct varMapNode *next;
} varMapNode;



#endif