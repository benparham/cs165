#include <stdlib.h>
#include <stdio.h>
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

	// Add room for the size of the entire serial
	serialAddSerialSizeInt(slzr);

	slzr->serial = (serializer *) malloc(slzr->serialSizeBytes);
	if (slzr->serial == NULL) {
		return 1;
	}

	// Write the size of the entire serial
	// Note: this size is inclusive of itself, whereas generally
	// 		 a size excludes itself
	serialWriteInt(slzr, slzr->serialSizeBytes);

	slzr->serialIsAlloc = 1;

	return 0;
}

int serializerSetSerial(serializer *slzr, void *serial) {
	if (slzr == NULL || serial == NULL) {
		return 1;
	}

	if (slzr->serialIsAlloc) {
		free(slzr->serial);
	}

	slzr->serial = serial;

	// Set this to pass the assert in serialRead
	slzr->serialSizeBytes = sizeof(int);
	serialReadInt(slzr, &(slzr->serialSizeBytes));

	slzr->serialIsAlloc = 0;

	return 0;
}

int serializerSetSerialFromFile(serializer *slzr, FILE *fp) {
	if (slzr == NULL || fp == NULL) {
		goto exit;
	}

	if (slzr->serialIsAlloc) {
		free(slzr->serial);
	}

	// Read in the size of the serial in bytes
	int sizeBytes;
	if (fread(&sizeBytes, sizeof(int), 1, fp) < 1) {
		goto exit;
	}

	// Seek back to beginning of serial
	if (fseek(fp, -sizeof(int), SEEK_CUR) == -1) {
		goto exit;
	}

	// Allocate space for the serial
	void *serial = malloc(sizeBytes);
	if (serial == NULL) {
		goto exit;
	}

	// Read in entire serial
	if (fread(serial, sizeBytes, 1, fp) < 1) {
		goto cleanupSerial;
	}

	// Set the size and serial to read in values
	slzr->serialSizeBytes = sizeBytes;
	slzr->serial = serial;

	// Set the offset to after the size and indicate serial was allocated
	slzr->offset = sizeof(int);
	slzr->serialIsAlloc = 1;

	return 0;

cleanupSerial:
	free(serial);
exit:
	return 1;
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