CC = gcc
CFLAGS = -Wall -Wextra 

SHARED_SRC := $(shell find src/shared -name "*.c") 
CLIENT_SRC := $(shell find src/client -name "*.c")
DAEMON_SRC := $(shell find src/daemon -name "*.c")

BIN_DIR = bin

DAEMON_BIN_NAME = abyssd
CLIENT_BIN_NAME = abyssctl

default:
	make client
	make daemon
	ls $(BIN_DIR)

clean:
	rm -rf bin

client:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(SHARED_SRC) -o $(BIN_DIR)/$(CLIENT_BIN_NAME)

daemon:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(DAEMON_SRC) $(SHARED_SRC) -o $(BIN_DIR)/$(DAEMON_BIN_NAME)
