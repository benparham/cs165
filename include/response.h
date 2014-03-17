#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#define RESPONSE(RES, NENTRIES, DATA) 		recordResponse(RES, NENTRIES, DATA)
#define RESPONSE_SUCCESS(RES) 				RESPONSE(RES, 0, NULL)

typedef struct response {
	int nEntries;
	int *data;
} response;

int responseCreate(response **res);
void responseDestroy(response *res);

void recordResponse(response *res, unsigned int nEntries, int *data);
int handleResponse(response *res, char **message);

#endif