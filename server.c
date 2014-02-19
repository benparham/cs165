#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BACKLOG 10
#define BUFSIZE 1024

#define LCL_LSTN_CMD "./listen.sh 1"
#define EXECUTE_CMD "./execute.sh "
#define SETUP_CMD "osascript setup.scpt"
#define HOST_LOOKUP_CMD "ifconfig | grep -P 'inet (?!127.0.0.1)'"

void setup(int socketFD);
void listen_to_client();
void listen_local(int socketFD);
void terminate_connection(int socketFD, int childPid);
int execute_command(char *command, int socketFD);

int main(int argc, char *argv[]) {
	printf("Initiating iTunes Remote Server2...\n\n");
	
	struct servent *serv;
	struct sockaddr_in addr;
	
	// Obtain http service
	serv  = getservbyname("http", "tcp");
	
	// Optional specification of port number
	int port_number;
	if (argc == 2) {
		port_number = 8888;
	}
	else {
		port_number = htons(serv->s_port);
	}
	
	// Create socket
	int sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_listen < 0) {
		printf("Failed to create listening socket\n");
		return 0;
	}
	else {
		printf("Created socket with file descriptor: %d\n", sock_listen);
	}
	
	// Setup the address (port number) to bind to
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port_number;//htons(serv->s_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	// Bind the socket to the address
	if (bind(sock_listen, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		printf("Successfully bound socket to port %d (%d)\n", ntohs(addr.sin_port), addr.sin_port);
	}
	else {
		printf("Failed to bind socket to port %d\n", ntohs(addr.sin_port));
		close(sock_listen);
		return 0;
	}
	
	// Listen on socket
	if (listen(sock_listen, BACKLOG) == 0) {
		printf("Listening on socket %d\n", sock_listen);

		// Get current hosting ip address
		FILE *ptr = popen(HOST_LOOKUP_CMD, "r");
		if (ptr != NULL) {
			printf("Host(s): ");

			char buf[BUFSIZE];
			char *success;
			do {
				memset(buf, 0, BUFSIZE);
				success = fgets(buf, BUFSIZE, ptr);
				if (buf != NULL) {
					printf("%s", buf);
				}
			} while(success != NULL);
		}
	}
	else {
		printf("Failed listen on socket %d\n", sock_listen);
		close(sock_listen);
		return 0;
	}
	
	struct sockaddr client_address;
	socklen_t address_len;
	
	// Loop for accepting connections
	while (1) {
		printf("\nWaiting for connection from client...\n");
		int sock_accept = accept(sock_listen, &client_address, &address_len);
		if (sock_accept == -1) {
			printf("Failed to accept connection on listening socket %d\n", sock_listen);
		}
		else {
			printf("Accepted new connection. Created socket with file descriptor: %d\n", sock_accept);
			setup(sock_accept);
			int pid = fork();
			if (pid == 0) {
				listen_local(sock_accept);
			}
			else {
				listen_to_client(sock_accept, pid);
			}
		}
	}
	
	close(sock_listen);
	return 0;
}

void setup(int socketFD) {
	FILE *ptr = popen(SETUP_CMD, "r");
	if (ptr != NULL) {
		char buf[BUFSIZE];
		memset(buf, 0, BUFSIZE);
		fgets(buf, BUFSIZE, ptr);
		if (buf != NULL) {
			strcat(buf, "\n");
				
			printf("Sending setup message: %s", buf);
			send(socketFD, buf, strlen(buf), 0);
		}
		else {
			printf("Return value from %s was null\n", SETUP_CMD);
		}
	}
	else {
		printf("Failed to run command %s\n", SETUP_CMD);
	}
	pclose(ptr);
}

void listen_to_client(int socketFD, int childPid) {
	char command_buffer[BUFSIZE];
	int bytes_recieved;
	
	while (1) {
		printf("Waiting to recieve data from client...\n");
		memset(command_buffer, 0, BUFSIZE);
		bytes_recieved = recv(socketFD, command_buffer, BUFSIZE, 0);
		if (bytes_recieved < 1) {
			printf("Client has closed connection\n");
			break;
		}
		else {
			printf("Data recieved: %s\n", command_buffer);
			execute_command(command_buffer, socketFD);
		}
	}
	
	terminate_connection(socketFD, childPid);
}

void listen_local(int socketFD) {
	char buf[BUFSIZE];
	FILE *ptr;
	
	while(1) {
		memset(buf, 0, BUFSIZE);
		ptr = popen(LCL_LSTN_CMD, "r");
		if (ptr != NULL) {
			fgets(buf, BUFSIZE, ptr);
			if (buf != NULL) {
				strcat(buf, "\n");
				
				printf("Sending message: %s", buf);
				send(socketFD, buf, strlen(buf), 0);
			}
			else {
				printf("Return value from %s was null\n", LCL_LSTN_CMD);
				break;
			}
		}
		else {
			printf("Failed to run command %s\n", LCL_LSTN_CMD);
			break;
		}
		pclose(ptr);
	}
}

void terminate_connection(int socketFD, int childPid) {
	printf("Terminating connection from server end...\n");
	printf("Closing socket %d\n", socketFD);
	close(socketFD);
	printf("Killing child process with pid: %d\n", childPid);
	kill(childPid, SIGTERM);
}

int execute_command(char *command, int socketFD) {
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	
	char final_cmd[BUFSIZE];
	memset(final_cmd, 0, BUFSIZE);
	strcpy(final_cmd, EXECUTE_CMD);
	strcat(final_cmd, command);
	
	FILE *ptr = popen(final_cmd, "r");
	if (ptr != NULL) {
		fgets(buf, BUFSIZE, ptr);
		if (buf != NULL) {
			if (strlen(buf) > 0) {
				strcat(buf, "\n");
				
				printf("Sending message: %s", buf);
				send(socketFD, buf, strlen(buf), 0);
			}
		}
		else {
			printf("Return value from %s was null\n", final_cmd);
		}
	}
	else {
		printf("Failed to run command %s\n", final_cmd);
	}
	pclose(ptr);
}

int execute_command_old(char *command, int socketFD) {
	char *strend = strchr(command, '\n');
	if (strend != NULL) {
		*strend = '\0';
	}
	
	char *filename = strtok(command, " ");
	char *remaining_args = strtok(NULL, "\0");
	
	int pid = fork();
	if (pid == 0) {
		execlp("osascript", "osascript", filename, remaining_args, NULL);
		exit(0);
	}
	else {
		waitpid(pid, NULL, 0);
	}
	
	return 0;
}