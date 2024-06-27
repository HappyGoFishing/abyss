CC = gcc
CFLAGS = -Wall

SHARED_SRC = src/shared/*.c
CLIENT_SRC = src/client/*.c
DAEMON_SRC = src/daemon/*.c

BIN_DIR = bin

DAEMON_BIN_NAME = daemon
CLIENT_BIN_NAME = client

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