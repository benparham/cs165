#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <serial.h>

int serializerCreate(serializer **slzr) {
	if (slzr == NULL) {
		return 1;
	}

	*slzr = (serializer *) malloc(sizeof(serializer));
	if (*slzr == NULL) {
		return 1;
	}

	(*slzr)->offset = 0;

	(*slzr)->serialIsAlloc = 0;
	(*slzr)->serialSizeBytes = 0;
	(*slzr)->serial = NULL;

	return 0;
}

int serializerAllocSerial(serializer *slzr) {
	if (slzr == NULL || slzr->serialSizeBytes < 1) {
		return 1;
	}

	slzr->serial = (serializer *) malloc(slzr->serialSizeBytes);
	if (slzr->serial == NULL) {
		return 1;
	}

	slzr->serialIsAlloc = 1;

	return 0;
}

int serializerSetSerial(serializer *slzr, void *serial, int sizeBytes) {
	if (slzr == NULL || serial == NULL) {
		return 1;
	}

	if (slzr->serialIsAlloc) {
		free(slzr->serial);
	}

	slzr->serial = serial;
	slzr->serialSizeBytes = sizeBytes;

	slzr->serialIsAlloc = 0;

	return 0;
}

void serializerDestroy(serializer *slzr) {
	if (slzr == NULL) {
		return;
	}

	if (slzr->serialIsAlloc) {
		free(slzr->serial);
	}

	free(slzr);
}




static void serialWrite(serializer *slzr, void *writeBuf, int nBytes) {
	assert(slzr->offset + nBytes <= slzr->serialSizeBytes);

	memcpy(slzr->serial + slzr->offset, writeBuf, nBytes);
	slzr->offset += nBytes;
}

static void serialRead(serializer *slzr, void *readBuf, int nBytes) {
	assert(slzr->offset + nBytes <= slzr->serialSizeBytes);

	memcpy(readBuf, slzr->serial + slzr->offset, nBytes);
	slzr->offset += nBytes;
}


// ================ Integers

void serialAddSerialSizeInt(serializer *slzr) {
	slzr->serialSizeBytes += sizeof(int);
}

void serialWriteInt(serializer *slzr, int intToWrite) {
	serialWrite(slzr, &intToWrite, sizeof(int));
}

void serialReadInt(serializer *slzr, int *intToRead) {
	serialRead(slzr, intToRead, sizeof(int));
}

// ================ Raw bytes

void serialAddSerialSizeRaw(serializer *slzr, int nBytes) {
	serialAddSerialSizeInt(slzr);
	slzr->serialSizeBytes += nBytes;
}

void serialWriteRaw(serializer *slzr, void *toWrite, int nBytes) {
	serialWriteInt(slzr, nBytes);
	serialWrite(slzr, toWrite, nBytes);
}

// Allocates *toRead
void serialReadRaw(serializer *slzr, void **toRead, int *bytesRead) {
	serialReadInt(slzr, bytesRead);

	*toRead = malloc(*bytesRead);
	if (*toRead == NULL) {
		return;
	}

	serialRead(slzr, *toRead, *bytesRead);
}

// ================ Strings

void serialAddSerialSizeStr(serializer *slzr, char *str) {
	// Don't forget terminating NULL
	int strBytes = (strlen(str) + 1) * sizeof(char);
	serialAddSerialSizeRaw(slzr, strBytes);
}

void serialWriteStr(serializer *slzr, char *strToWrite) {
	// Don't forget terminating NULL
	int strBytes = (strlen(strToWrite) + 1) * sizeof(char);
	serialWriteRaw(slzr, strToWrite, strBytes);
}

// Allocates *strToRead via serialReadRaw
void serialReadStr(serializer *slzr, char **strToRead) {
	int bytesRead;
	serialReadRaw(slzr, (void **) strToRead, &bytesRead);

	assert(bytesRead = (strlen(*strToRead) + 1) * sizeof(char));
}