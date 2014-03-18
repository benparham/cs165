#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <message.h>
#include <global.h>

#define SRL_MSG_NON_STR_BYTES		2 * sizeof(int)
#define SRL_DATA_NON_DATA_BYTES		sizeof(int)


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


typedef struct rawData {
	int dataBytes;
	void *data;
} rawData;


static int rawDataCreate(rawData **rData) {
	*rData = (rawData *) malloc(sizeof(rawData));
	if (*rData == NULL) {
		return 1;
	}

	(*rData)->dataBytes = 0;
	(*rData)->data = NULL;

	return 0;
}

static void rawDataDestroy(rawData *rData) {
	if (rData == NULL) {
		return;
	}

	if (rData->data != NULL) {
		free(rData->data);
	}

	free(rData);
}


static void writeToSerial(void *serial, int *offset, void *toWrite, int nBytes) {
	memcpy(serial + (*offset), toWrite, nBytes);
	*offset += nBytes;
}

static void readFromSerial(void *serial, int *offset, void *readBuf, int nBytes) {
	memcpy(readBuf, serial + (*offset), nBytes);
	*offset += nBytes;
}


// Allocates *serial
static int messageSerialize(message *msg, void **serial, int *serialBytes) {
	if (msg == NULL) {
		return 1;
	}

	int msgStrLen = strlen(msg->msgStr);

	// int intBytes = 2 * sizeof(int);
	int strBytes = msgStrLen * sizeof(char);
	*serialBytes = SRL_MSG_NON_STR_BYTES + strBytes;

	*serial = malloc(*serialBytes);
	if (*serial == NULL) {
		return 1;
	}

	int offset = 0;
	writeToSerial(*serial, &offset, &(msg->hasData), sizeof(int));
	writeToSerial(*serial, &offset, &msgStrLen, sizeof(int));
	writeToSerial(*serial, &offset, msg->msgStr, msgStrLen);

	return 0;
}

// Allocates msg->msgStr
static int messageDeserialize(void *serial, int serialBytes, message *msg) {
	int minSerialSize = SRL_MSG_NON_STR_BYTES + (sizeof(unsigned char));
	if (serial == NULL || serialBytes < minSerialSize) {
		return 1;
	}

	int hasData;
	int msgStrLen;

	int offset = 0;
	readFromSerial(serial, &offset, &hasData, sizeof(int));
	readFromSerial(serial, &offset, &msgStrLen, sizeof(int));

	if (msgStrLen != serialBytes - SRL_MSG_NON_STR_BYTES) {
		return 1;
	}

	char *msgStr = (char *) malloc((msgStrLen * sizeof(char)) + 1);
	if (msgStr == NULL) {
		return 1;
	}

	readFromSerial(serial, &offset, msgStr, msgStrLen);

	msg->hasData = hasData;
	msg->msgStr = msgStr;

	return 0;
}

static int rawDataSerialize(rawData *rData, void **serial, int *serialBytes) {
	if (rData == NULL) {
		return 1;
	}

	*serialBytes = sizeof(int) + rData->dataBytes;

	*serial = malloc(*serialBytes);
	if (*serial == NULL) {
		return 1;
	}

	int offset = 0;
	writeToSerial(*serial, &offset, &(rData->dataBytes), sizeof(int));
	writeToSerial(*serial, &offset, rData->data, rData->dataBytes);

	return 0;
}

static int rawDataDeserialize(void *serial, int serialBytes, rawData *rData) {
	int minSerialSize = SRL_DATA_NON_DATA_BYTES + sizeof(unsigned char);
	if (serial == NULL || serialBytes < minSerialSize) {
		return 1;
	}

	int dataBytes;

	int offset = 0;
	readFromSerial(serial, &offset, &dataBytes, sizeof(int));

	if (dataBytes != serialBytes - SRL_DATA_NON_DATA_BYTES) {
		return 1;
	}

	void *data = malloc(dataBytes);
	if (data == NULL) {
		return 1;
	}

	readFromSerial(serial, &offset, data, dataBytes);

	rData->dataBytes = dataBytes;
	rData->data = data;

	return 0;
}


static int messageSendDataFlag(int socketFD, char *msgStr, int hasData) {
	message *msg;
	if (messageCreate(&msg)) {
		goto exit;
	}

	msg->hasData = hasData;
	msg->msgStr = msgStr;

	void *serial;
	int serialBytes;
	if (messageSerialize(msg, &serial, &serialBytes)) {
		goto cleanupMsg;
	}

	// Send serial over socket
	send(socketFD, serial, serialBytes, 0);

	free(serial);
	messageDestroy(msg);

	return 0;

cleanupMsg:
	messageDestroy(msg);
exit:
	return 1;
}

int messageSend(int socketFD, char *msgStr) {
	return messageSendDataFlag(socketFD, msgStr, 0);
}

int dataSend(int socketFD, char *msgStr, int dataBytes, void *data) {
	rawData *rData;
	if (rawDataCreate(&rData)) {
		goto exit;
	}

	rData->dataBytes = dataBytes;
	rData->data = data;

	void *serial;
	int serialBytes;
	if (rawDataSerialize(rData, &serial, &serialBytes)) {
		goto cleanupRawData;
	}

	if (messageSendDataFlag(socketFD, msgStr, 1)) {
		goto cleanupSerial;
	}

	send(socketFD, serial, serialBytes, 0);

	free(serial);
	rawDataDestroy(rData);

	return 0;

cleanupSerial:
	free(serial);
cleanupRawData:
	rawDataDestroy(rData);
exit:
	return 1;
}

static int dataReceive(int socketFD, int *dataBytes, void **data) {

	unsigned char serial[BUFSIZE];
	memset(serial, 0, BUFSIZE);

	int minSerialSize = SRL_DATA_NON_DATA_BYTES + sizeof(unsigned char);

	int bytesRecieved = recv(socketFD, serial, BUFSIZE, 0);
	if (bytesRecieved < minSerialSize) {
		goto exit;
	}

	rawData *rData;
	if (rawDataCreate(&rData)) {
		goto exit;
	}
	if (rawDataDeserialize(serial, bytesRecieved, rData)) {
		goto cleanupRawData;
	}

	*dataBytes = rData->dataBytes;
	*data = rData->data;

	rawDataDestroy(rData);

	return 0;

cleanupRawData:
	rawDataDestroy(rData);
exit:
	return 1;
}

int messageReceive(int socketFD, char *msgStr, int *dataBytes, void **data) {

	unsigned char serial[BUFSIZE];
	memset(serial, 0, BUFSIZE);

	int minSerialSize = SRL_MSG_NON_STR_BYTES + sizeof(unsigned char);

	// Get serialized message
	int bytesRecieved = recv(socketFD, serial, BUFSIZE, 0);
	if (bytesRecieved < minSerialSize) {
		goto exit;
	}

	// Deserialize into message
	message *msg;
	if (messageCreate(&msg)) {
		goto exit;
	}
	if (messageDeserialize(serial, bytesRecieved, msg)) {
		goto cleanupMessage;
	}

	//*msgStr = msg->msgStr;
	strcpy(msgStr, msg->msgStr);

	if (msg->hasData) {
		if (dataReceive(socketFD, dataBytes, data)) {
			goto cleanupMessage;
		}
	} else {
		*dataBytes = 0;
		*data = NULL;
	}

	messageDestroy(msg);

	return 0;

cleanupMessage:
	messageDestroy(msg);
exit:
	return 1;
}