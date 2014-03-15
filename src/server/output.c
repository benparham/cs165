#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "output.h"

#define MAX_MSG_HDR_SIZE		64

// Auto generate the type of error, allow format ... for the message. Yeah
void genError() {

}


void printMsg(int threadId, const char *restrict format, ...) {
	va_list args;
	va_start(args, format);

	char newFormat[MAX_MSG_HDR_SIZE + sizeof(format)];
	sprintf(newFormat, "Thread %d: %s", threadId, format);

	vprintf(newFormat, args);
}