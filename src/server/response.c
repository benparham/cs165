#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <response.h>

int responseCreate(response **res) {
	*res = (response *) malloc(sizeof(response));
	if (res == NULL) {
		return 1;
	}

	(*res)->nEntries = -1;
	(*res)->data = NULL;

	return 0;
}

void responseDestroy(response *res) {
	if (res == NULL) {
		return;
	}

	if (res->data != NULL) {
		free(res->data);
	}

	free(res);
}

// Should only be called after handle response, never twice in a row, thus the asserts
void recordResponse(response *res, unsigned int nEntries, int *data) {
	assert(res->nEntries == -1);
	assert(res->data == NULL);

	if (nEntries == 0) {
		assert(data == NULL);

		res->nEntries = 0;
		res->data = NULL;
		return;
	}

	res->nEntries = nEntries;
	res->data = data;
}

int handleResponse(response *res, char **message) {
	int result;

	if (res->nEntries < 0) {
		
		*message = "Response error, terminating connection";
		result = 1;

	} else if (res->nEntries == 0) {

		assert(res->data == NULL);

		*message = "Success";
		result = 0;

	} else {

		// int length = res->nEntries;
		// for (int i = 0; i < length; i++) {

		// }
		// strcat(*message, "Data");
		*message = "Data";
		result = 0;

	}

	res->nEntries = -1;
	res->data = NULL;

	printf("Response: %s\n", *message);

	return result;
}