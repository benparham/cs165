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

	(*res)->dataBytes = -1;
	(*res)->message = NULL;
	(*res)->data = NULL;

	return 0;
}

void responseWipe(response *res) {
	if (res == NULL) {
		return;
	}

	res->dataBytes = -1;
	
	if (res->message != NULL) {
		free(res->message);
		res->message = NULL;
	}

	if (res->data != NULL) {
		free(res->data);
		res->data = NULL;
	}
}

void responseDestroy(response *res) {
	if (res == NULL) {
		return;
	}

	responseWipe(res);

	free(res);
}

// Should only be called after handle response, never twice in a row, thus the asserts
void recordResponse(response *res, char *message, unsigned int dataBytes, void *data) {
	assert(res->dataBytes == -1);
	assert(res->message == NULL);
	assert(res->data == NULL);

	assert(message != NULL);
	assert(dataBytes >= 0);
	if (dataBytes == 0) {
		assert(data == NULL);
	} else {
		assert(data != NULL);
	}

	res->dataBytes = dataBytes;
	// res->message = message;
	res->data = data;

	res->message = (char *) malloc((strlen(message) + 1) * sizeof(char));
	if (res->message == NULL) {
		goto exit;
	}
	strcpy(res->message, message);

	// res->data = malloc(dataBytes);
	// if (res->data == NULL) {
	// 	goto exit;
	// }
	// memcpy(res->data, data, dataBytes);

	return;

exit:
	res->dataBytes = 0;
	return;
}

// int handleResponse(response *res, char **message, int *dataBytes, void **data) {
// 	int result;

// 	if (res->nEntries < 0) {
		
// 		*message = "Response error, terminating connection";
// 		result = 1;

// 	} else if (res->nEntries == 0) {

// 		assert(res->data == NULL);

// 		*message = "Success";
// 		result = 0;

// 	} else {

// 		// int length = res->nEntries;
// 		// for (int i = 0; i < length; i++) {

// 		// }
// 		// strcat(*message, "Data");
// 		*message = "Data";

// 		*dataBytes

// 		result = 0;

// 	}

// 	res->nEntries = -1;
// 	res->data = NULL;

// 	#ifdef DEBUG
// 	printf("Response: %s\n", *message);
// 	#endif

// 	return result;
// }