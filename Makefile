all: server client

client:client.c
	gcc client.c -o client -std=c99

server:server.c
	gcc server.c -o server -std=c99