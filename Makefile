all: server client

client:client.c
	gcc client.c -o client -std=c99

server:server.c database.c dberror.c command.c output.c filesys.c debug.c
	gcc server.c database.c dberror.c command.c output.c filesys.c debug.c -o server -std=c99