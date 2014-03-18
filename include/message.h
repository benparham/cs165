#ifndef _MESSAGE_H_
#define _MESSAGE_H_


int messageSend(int socketFD, char *msgStr);
int dataSend(int socketFD, char *msgStr, int dataBytes, void *data);

int messageReceive(int socketFD, char *msgStr, int *dataBytes, void **data, int *term);

#endif