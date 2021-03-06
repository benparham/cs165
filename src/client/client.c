#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>

#include <message.h>

#define DEFAULT_TARGET_ADDRESS "127.0.0.1"
#define MAX_INPUT 1024

#define DEFAULT_SCRIPT_FOLDER 	"scripts/p2/"

static int loadFromCsv(char *fileName, int socketFD) {

	char *defaultParent = DEFAULT_SCRIPT_FOLDER;
	char *total = (char *) malloc((strlen(defaultParent) + strlen(fileName) + 1) * sizeof(char));
	if (total == NULL) {
		printf("No memory\n");
		goto exit;
	}
	strcpy(total, defaultParent);
	strcat(total, fileName);

	// Open file
	FILE *fp = fopen(total, "r");
	if (fp == NULL) {
		printf("Could not open '%s'\n", total);
		goto cleanupTotal;
	}

	// Line buffer (can be resized by getline)
	size_t lineBytes = MAX_INPUT;
	char *line = (char *) malloc(lineBytes);
	if (line == NULL) {
		printf("Out of memory\n");
		goto cleanupFile;
	}

	// Read in column names
	int charsRead = getline(&line, &lineBytes, fp);
	if (charsRead < 1) {
		printf("Malformed data in file '%s'\n", total);
		goto cleanupLine;
	}

	// Store the comma separated list of columns and the number of columns
	char *columnNames = (char *) malloc(charsRead * sizeof(char));
	if (columnNames == NULL) {
		printf("Out of memory\n");
		goto cleanupLine;
	}
	strcpy(columnNames, line);

	// Remove trailing newline
	strtok(columnNames, "\n");

	int numColumns = 1;
	for (int i = 0; i < charsRead; i++) {
		if (columnNames[i] == ',') {
			numColumns += 1;
		}
	}

	
	int numRows = 0;


	// Read csv entries into data buffer
	int rowBytes = numColumns * sizeof(int);
	int currentSizeBytes = 0;
	void *data = NULL;
	int dataOffset = 0;
	while((charsRead = getline(&line, &lineBytes, fp)) > 0) {
		currentSizeBytes += rowBytes;
		void *temp = realloc(data, currentSizeBytes);
		if (temp == NULL) {
			printf("Out of memory");
			goto cleanupData;
		}

		data = temp;

		char *entry = strtok(line, ",\n");
		int numEntries = 0;
		while (entry != NULL) {
			int intEntry = atoi(entry);
			memcpy(data + dataOffset, &intEntry, sizeof(int));
			dataOffset += sizeof(int);

			numEntries += 1;

			entry = strtok(NULL, ",\n");
		}

		if (numEntries != numColumns) {
			printf("Malformed data: row %d\n", numRows);
			goto cleanupData;
		}

		numRows += 1;
	}

	// Send message and data to server
	char *msgBeg = "load(";
	char *message = (char *) malloc((strlen(msgBeg) + strlen(columnNames) + 2) * sizeof(char));
	sprintf(message, "%s%s)", msgBeg, columnNames);

	if (dataSend(socketFD, message, currentSizeBytes, data)) {
		printf("->: Error sending message\n");
		goto cleanupMessage;
	}


	// Cleanup
	free(message);
	if (data != NULL) {
		free(data);
	}
	free(columnNames);
	free(line);
	fclose(fp);

	return 0;

cleanupMessage:
	free(message);
cleanupData:
	if (data != NULL) {
		free(data);
	}
// cleanupColumnNames:
	free(columnNames);
cleanupLine:
	free(line);
cleanupFile:
	fclose(fp);
cleanupTotal:
	free(total);
exit:
	return 1;
}

static void getInput(int socketFD) {

	// Detect whether we are in interactive mode
	int inAtty = isatty(0);
	// int outAtty = isatty(1);

	char *input = malloc(sizeof(char) * MAX_INPUT);
	char *response = malloc(sizeof(char) * MAX_INPUT);
	
	while(1) {
		memset(input, 0, sizeof(input));
		memset(response, 0, sizeof(response));
		
		// Prompt/get input
		if (inAtty) {
			printf(">: ");
		}
		fgets(input, MAX_INPUT, stdin);

		if (strcmp(input, "") == 0) {
			break;
		}
		if (strcmp(input, "\n") == 0) {
			printf("\r");
			continue;
		}

		// Display input if not in interactive mode
		// if (!inAtty) {
		// 	printf("%s", input);
		// }

		int skipReceive = 0;

		char *loadCommand = "load(\"";
		int loadCommandLen = strlen(loadCommand);
		if (strncmp(input, loadCommand, loadCommandLen) == 0) {
			skipReceive = loadFromCsv(strtok(&input[loadCommandLen], "\")\n"), socketFD);
		} else {
			if (messageSend(socketFD, input)) {
				printf("->: Error sending message\n");
				skipReceive = 1;
			}

			if (strcmp(input, "exit\n") == 0) {
				break;
			}
		}

		if (!skipReceive) {
			int dataBytes;
			void *data;
			int term;
			if (messageReceive(socketFD, response, &dataBytes, &data, &term)) {
				if (term) {
					printf("->: Server has exited\n");
					break;
				} else {
					printf("->: Error receiving message\n");
				}
			}

			// If not in interactive mode, just print data results
			if (inAtty) {
				printf("->: %s\n", response);
			}

			if (dataBytes > 0) {
				
				assert(dataBytes % sizeof(int) == 0);
				int nEntries = dataBytes / sizeof(int);

				if (!inAtty) {
					printf("%s", response);
				}
				printf("->: [");

				for (int i = 0; i < nEntries; i++) {
					printf("%d", ((int *) data)[i]);

					if (i != nEntries - 1) {
						printf(",");
					}
				}

				printf("]\n");

				free(data);
			}
		}
	}

	free(input);
	free(response);
}

int main(int argc, char *argv[]) {
	printf("Initiating Client...\n");
	
	struct servent *serv;
	struct sockaddr_in addr;
	char *target_address;
	
	// Get address to connect to
	if (argc == 2) {
		target_address = argv[1];
	}
	else {
		target_address = DEFAULT_TARGET_ADDRESS;
	}
	
	// Obtain http service
	serv = getservbyname("http", "tcp");
	
	// Create socket
	int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD == -1) {
		printf("Failed to create socket\n");
		return 0;
	}
	else {
		printf("Created socket with file descriptor: %d\n", socketFD);
	}
	
	// Setup address to connect to
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv->s_port);
	endservent();	 // closes etc/services
	if (inet_pton(AF_INET, target_address, &addr.sin_addr) != 1) {
		printf("Failed to convert string %s to a network address\n", target_address);
		close(socketFD);
		return 0;
	}
	
	// Connect to address
	printf("Attempting to connect to address %s...\n", target_address);
	if (connect(socketFD, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		printf("Connected to address %s\n", target_address);
	}
	else {
		printf("Failed to connect to address %s\n", target_address);
		close(socketFD);
		return 0;
	}

	printf("\n");

	// Hack to always use table test
	char *defaultTable = "use test";
	if (messageSend(socketFD, defaultTable)) {
		close(socketFD);
		printf("Couldn't send message to use table %s\n", defaultTable);
		return 1;
	}
	char *expected = "Success";
	char *response = (char *) malloc((strlen(expected) + 1) * sizeof(char));
	if (response == NULL) {
		close(socketFD);
		printf("Not memory\n");
		return 1;
	}
	int dataBytes;
	void *data;
	int term;
	if (messageReceive(socketFD, response, &dataBytes, &data, &term)) {
		close(socketFD);
		printf("Bad thing1\n");
		return 1;
	}
	if (dataBytes > 0 || strcmp(response, expected) != 0) {
		close(socketFD);
		printf("Bad thing2\n");
		return 1;
	}

	getInput(socketFD);
	
	// Cleanup socket
	close(socketFD);
	printf("Closed socket %d\n", socketFD);
	
	return 0;
}