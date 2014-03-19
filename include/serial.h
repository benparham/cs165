#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdio.h>

typedef struct serializer {
	int offset;

	int serialIsAlloc;
	int serialSizeBytes;
	void *serial;
} serializer;

int serializerCreate(serializer **slzr);
int serializerAllocSerial(serializer *slzr);
int serializerSetSerial(serializer *slzr, void *serial);
int serializerSetSerialFromFile(serializer *slzr, FILE *fp);
void serializerDestroy(serializer *slzr);

void serialAddSerialSizeInt(serializer *slzr);
void serialWriteInt(serializer *slzr, int intToWrite);
void serialReadInt(serializer *slzr, int *intToRead);

void serialAddSerialSizeRaw(serializer *slzr, int nBytes);
void serialWriteRaw(serializer *slzr, void *toWrite, int nBytes);
void serialReadRaw(serializer *slzr, void **toRead, int *bytesRead);

void serialAddSerialSizeStr(serializer *slzr, char *str);
void serialWriteStr(serializer *slzr, char *strToWrite);
void serialReadStr(serializer *slzr, char **strToRead);

#endif