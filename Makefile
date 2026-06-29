CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
LDFLAGS = 

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/shell.c
HEADERS = $(SRC_DIR)/shell.h
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/shell.o
TARGET = $(BIN_DIR)/shell

.PHONY: all clean run debug valgrind

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build successful: $(TARGET)"

run: $(TARGET)
	./$(TARGET)

# Debug with gdb
debug: CFLAGS += -O0
debug: $(TARGET)
	gdb ./$(TARGET)

# Check for memory leaks with Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Trace system calls with strace
strace: $(TARGET)
	strace -e trace=fork,execve,wait4 ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✓ Clean successful"

help:
	@echo "Available targets:"
	@echo "  make           - Build the shell"
	@echo "  make run       - Build and run the shell"
	@echo "  make debug     - Build and run with gdb"
	@echo "  make valgrind  - Check for memory leaks"
	@echo "  make strace    - Trace system calls"
	@echo "  make clean     - Remove build artifacts"
