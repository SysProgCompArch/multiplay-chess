# Makefile for multiplay-chess project

# Basic variables
BUILD_DIR = build
CMAKE_BUILD_TYPE = Release

# Default target (when running just 'make')
.PHONY: all clean client server test-client help deps

all: client server test-client

# Check dependencies
deps:
	@echo "Checking dependencies..."
	@for pkg in pkg-config protobuf-compiler protobuf-c-compiler libprotobuf-dev libprotobuf-c-dev libncurses-dev; do \
		if ! dpkg -s "$$pkg" >/dev/null 2>&1; then \
			echo "Error: Required package $$pkg is not installed."; \
			echo "Please install dependencies with the following command:"; \
			echo "  sudo apt install pkg-config protobuf-compiler protobuf-c-compiler libprotobuf-dev libprotobuf-c-dev libncurses-dev"; \
			exit 1; \
		fi \
	done
	@echo "All dependencies are installed."

# CMake configuration (create and initialize build directory)
$(BUILD_DIR)/Makefile: CMakeLists.txt
	@echo "Generating build system using CMake..."
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) ..

# Individual target builds
client: deps $(BUILD_DIR)/Makefile
	@echo "Building client..."
	cd $(BUILD_DIR) && make client
	@echo "Client build completed: $(BUILD_DIR)/client/client"

server: deps $(BUILD_DIR)/Makefile
	@echo "Building server..."
	cd $(BUILD_DIR) && make server
	@echo "Server build completed: $(BUILD_DIR)/server/server"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean completed."

# Run commands (convenience features)
run-server: server
	@echo "Starting server..."
	./$(BUILD_DIR)/server/server

run-client: client
	@echo "Starting client..."
	./$(BUILD_DIR)/client/client

# Help
help:
	@echo "Available make targets:"
	@echo "  all         - Build all targets (client, server, test-client)"
	@echo "  client      - Build client only"
	@echo "  server      - Build server only"
	@echo "  clean       - Clean build artifacts"
	@echo "  deps        - Check dependencies"
	@echo "  run-server  - Build and run server"
	@echo "  run-client  - Build and run client"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build all"
	@echo "  make client       # Build client only"
	@echo "  make clean        # Clean build artifacts" 