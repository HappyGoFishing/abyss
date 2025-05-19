CC = gcc
CFLAGS = -Wall -Wextra 

VENDOR_SRC := $(shell find src/vendor -name "*.c") 
CLIENT_SRC := $(shell find src/client -name "*.c")
DAEMON_SRC := $(shell find src/daemon -name "*.c")

BIN_DIR = out

DAEMON_BIN_NAME = abyssd
CLIENT_BIN_NAME = abyssctl

default:
	make client
	make daemon
	ls $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

client:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) $(VENDOR_SRC) -o $(BIN_DIR)/$(CLIENT_BIN_NAME)

daemon:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(DAEMON_SRC) $(VENDOR_SRC) -o $(BIN_DIR)/$(DAEMON_BIN_NAME)
