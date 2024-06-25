BIN_DIR = bin
SRC_DIR = src
BIN_NAME = abyssctl

CC = cc
CFLAGS = -Wall -Wextra

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC_FILES))

default: $(BIN_DIR)/$(BIN_NAME)

$(BIN_DIR)/$(BIN_NAME): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: default clean
BIN_DIR = bin
SRC_DIR = src
BIN_NAME = abyss_watcher

CC = cc
CFLAGS = -Wall -Wextra

SRC_FILES = "$()"
