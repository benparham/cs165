all: server client

client:client.c
	gcc client.c -o client -std=c99

server:server.c database.c command.c filesys.c error.c table.c column.c data.c
	gcc server.c database.c command.c filesys.c error.c table.c column.c data.c -o server -std=c99