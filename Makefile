all: server client

client:client.c
	gcc client.c -o client -std=c99

server:server.c database.c dberror.c
	gcc server.c database.c dberror.c -o server -std=c99