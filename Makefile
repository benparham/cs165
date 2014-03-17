CC = gcc

INCLUDE_DIR = include
SOURCE_DIR = src
BUILD_DIR = build

SERVER = server
CLIENT = client
COMMON = common

SERV_SOURCE_DIR = $(SOURCE_DIR)/$(SERVER)
SERV_BUILD_DIR = $(BUILD_DIR)/$(SERVER)

CLIENT_SOURCE_DIR = $(SOURCE_DIR)/$(CLIENT)
CLIENT_BUILD_DIR = $(BUILD_DIR)/$(CLIENT)

COMMON_SOURCE_DIR = $(SOURCE_DIR)/$(COMMON)
COMMON_BUILD_DIR = $(BUILD_DIR)/$(COMMON)

# Flags
CFLAGS = -g -Wall -std=c99 -I $(INCLUDE_DIR)

# Header files
INC = $(shell find $(INCLUDE_DIR) -type f -name '*.h')

# Common files
COMMON_SRC = $(shell find $(COMMON_SOURCE_DIR) -type f -name '*.c')

# Server files
SERV_SRC = $(shell find $(SERV_SOURCE_DIR) -type f -name '*.c') $(COMMON_SRC)
SERV_OBJ = $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SERV_SRC))

# Client files
CLIENT_SRC = $(shell find $(CLIENT_SOURCE_DIR) -type f -name '*.c') $(COMMON_SRC)
CLIENT_OBJ = $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(CLIENT_SRC))


# Rules #######################

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ 

$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(INC)
#$(INC)
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	@rm -rf $(BUILD_DIR)/*
	@rm -f $(SERVER)
	@rm -f $(CLIENT)

###############################