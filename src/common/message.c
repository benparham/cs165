#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <message.h>
#include <global.h>
#include <serial.h>

typedef struct message {
	int hasData;
	char *msgStr;
} message;

static int messageCreate(message **msg) {
	*msg = (message *) malloc(sizeof(message));
	if (*msg == NULL) {
		return 1;
	}

	(*msg)->hasData = 0;
	(*msg)->msgStr = NULL;

	return 0;
}

static void messageDestroy(message *msg) {
	if (msg == NULL) {
		return;
	}

	if (msg->msgStr != NULL) {
		free(msg->msgStr);
	}

	free(msg);
}

static int messageSerialize(message *msg, serializer *slzr) {
	if (msg == NULL || slzr == NULL) {
		return 1;
	}

	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeStr(slzr, msg->msgStr);

	if (serializerAllocSerial(slzr)) {
		return 1;
	}

	serialWriteInt(slzr, msg->hasData);
	serialWriteStr(slzr, msg->msgStr);

	return 0;
}

static int messageDeserialize(message *msg, serializer *slzr) {

	if (msg == NULL || slzr == NULL) {
		return 1;
	}

	serialReadInt(slzr, &(msg->hasData));
	serialReadStr(slzr, &(msg->msgStr));

	return 0;
}



static int messageSendDataFlag(int socketFD, char *msgStr, int hasData) {
	// Create message
	message *msg;
	if (messageCreate(&msg)) {
		goto exit;
	}

	// Initialize
	msg->hasData = hasData;
	msg->msgStr = (char *) malloc(strlen(msgStr) * sizeof(char));
	strcpy(msg->msgStr, msgStr);

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		goto cleanupMsg;
	}

	// Serialize message
	if (messageSerialize(msg, slzr)) {
		goto cleanupSerial;
	}

	// Send serial over socket
	send(socketFD, slzr->serial, slzr->serialSizeBytes, 0);

	// Cleanup message and serializer
	serializerDestroy(slzr);
	messageDestroy(msg);

	return 0;

// Handle errors
cleanupSerial:
	serializerDestroy(slzr);
cleanupMsg:
	messageDestroy(msg);
exit:
	return 1;
}

int dataSend(int socketFD, char *msgStr, int dataBytes, void *data) {

	// Create serailizer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		goto exit;
	}

	// Serialize data
	serialAddSerialSizeRaw(slzr, dataBytes);
	serializerAllocSerial(slzr);
	serialWriteRaw(slzr, data, dataBytes);

	// Send message with data flag set
	if (messageSendDataFlag(socketFD, msgStr, 1)) {
		goto cleanupSerial;
	}

	// Send serialized data
	send(socketFD, slzr->serial, slzr->serialSizeBytes, 0);

	// Cleanup serializer
	serializerDestroy(slzr);

	return 0;

// Handle errors
cleanupSerial:
	serializerDestroy(slzr);
exit:
	return 1;
}

int messageSend(int socketFD, char *msgStr) {
	return messageSendDataFlag(socketFD, msgStr, 0);
}

static int dataReceive(int socketFD, int *dataBytes, void **data, int *term) {

	// Set up serial buffer
	unsigned char serial[BUFSIZE];
	memset(serial, 0, BUFSIZE);

	// Receive serialized data
	int bytesReceived = recv(socketFD, serial, BUFSIZE, 0);
	if (bytesReceived < 1) {
		*term = 1;
		goto exit;
	}

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		goto exit;
	}

	// Init serializer with received data
	if (serializerSetSerial(slzr, serial/*, bytesReceived*/)) {
		goto cleanupSerial;
	}

	// Deserialize data
	serialReadRaw(slzr, data, dataBytes);
	
	// Cleanup serializer
	serializerDestroy(slzr);

	return 0;

// Handle errors
cleanupSerial:
	serializerDestroy(slzr);
exit:
	return 1;
}

int messageReceive(int socketFD, char *msgStr, int *dataBytes, void **data, int *term) {

	// Flag for terminated connection
	*term = 0;

	// Set up serial buffer
	unsigned char serial[BUFSIZE];
	memset(serial, 0, BUFSIZE);

	// Receive serialized message
	int bytesReceived = recv(socketFD, serial, BUFSIZE, 0);
	if (bytesReceived < 1) {
		*term = 1;
		goto exit;
	}

	// Create message
	message *msg;
	if (messageCreate(&msg)) {
		goto exit;
	}

	// Create serializer
	serializer *slzr;
	if (serializerCreate(&slzr)) {
		goto cleanupMessage;
	}

	// Init serializer with received message
	if (serializerSetSerial(slzr, serial/*, bytesReceived*/)) {
		goto cleanupMessage;
	}

	// Deseriailize message
	if (messageDeserialize(msg, slzr)) {
		goto cleanupSerial;
	}

	// TODO: change so that we no longer need this (i.e. should not be passed an allocated char *)
	strcpy(msgStr, msg->msgStr);

	// Receive data if data flag is set in message
	if (msg->hasData) {
		if (dataReceive(socketFD, dataBytes, data, term)) {
			goto cleanupSerial;
		}
	} else {
		*dataBytes = 0;
		*data = NULL;
	}

	// Cleanup serializer and message
	serializerDestroy(slzr);
	messageDestroy(msg);

	return 0;

// Handle errors
cleanupSerial:
	serializerDestroy(slzr);
cleanupMessage:
	messageDestroy(msg);
exit:
	return 1;
}