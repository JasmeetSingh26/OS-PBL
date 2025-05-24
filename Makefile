# Compiler and flags
CC = gcc
OPENSSL_PATH = /opt/homebrew/opt/openssl@3

CFLAGS = -Wall -I$(OPENSSL_PATH)/include
LDFLAGS = -L$(OPENSSL_PATH)/lib -lssl -lcrypto -lm -lpthread

# Executable names
CLIENT_EXEC = client
SERVER_EXEC = server

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Test script
TEST_SCRIPT = tests.sh

# Default target
.PHONY: all
all: build

# Compile individual components
compile-client:
	$(CC) $(CLIENT_SRC) -o $(CLIENT_EXEC) $(CFLAGS) $(LDFLAGS)

compile-server:
	$(CC) $(SERVER_SRC) -o $(SERVER_EXEC) $(CFLAGS) $(LDFLAGS)

# Build both client and server
.PHONY: build
build: compile-client compile-server
	@echo "Build completed successfully!"

# Run the application and tests
.PHONY: run
run: build
	@chmod +x $(TEST_SCRIPT)
	@echo "Running tests..."
	./$(TEST_SCRIPT)

# Clean up generated files
.PHONY: clean
clean:
	@echo "Cleaning up build files..."
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)
	@echo "Cleanup completed!"
