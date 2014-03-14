# Compiler
CC = gcc

# Source
SDIR = src
# Include
IDIR = include
# Object
ODIR = $(SDIR)/obj

# Flags
CFLAGS = -g -Wall -std=c99 -I $(IDIR)

# Header files
INC = $(wildcard $(IDIR)/*.h)

# Server dependencies
_SERV_OBJ = server.o database.o command.o filesys.o error.o table.o column.o insert.o
SERV_OBJ = $(patsubst %, $(ODIR)/%, $(_SERV_OBJ))

# Client dependencies
_CLIENT_OBJ = client.o
CLIENT_OBJ = $(patsubst %, $(ODIR)/%, $(_CLIENT_OBJ))

# Rules #######################

all: server client

server: $(SERV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.c $(INC)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(ODIR)/*.o
	rm -f server
	rm -f client

###############################