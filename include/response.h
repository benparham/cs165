#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#define RESPONSE(RES, MESSAGE, NENTRIES, DATA) 		recordResponse(RES, MESSAGE, NENTRIES, DATA)
#define RESPONSE_NO_DATA(RES, MESSAGE)				RESPONSE(RES, MESSAGE, 0, NULL)
#define RESPONSE_SUCCESS(RES) 						RESPONSE_NO_DATA(RES, "Success")

typedef struct response {
	char *message;
	int dataBytes;
	void *data;
} response;

int responseCreate(response **res);
void responseWipe(response *res);
void responseDestroy(response *res);

void recordResponse(response *res, char *message, unsigned int dataBytes, void *data);
// int handleResponse(response *res, char **message, int *dataBytes, void **data);

#endif