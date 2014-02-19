all: server client

client:
	gcc client.c -o client -std=c99

server:
	gcc server.c -o server -std=c99