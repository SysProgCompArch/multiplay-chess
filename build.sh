#!/bin/sh

# Check for required dependencies
for pkg in pkg-config protobuf-compiler protobuf-c-compiler libprotobuf-dev libprotobuf-c-dev libncurses-dev; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        echo "Error: Required package $pkg is not installed.\nPlease install the dependencies and try again:\n  sudo apt install pkg-config protobuf-compiler protobuf-c-compiler libprotobuf-dev libprotobuf-c-dev libncurses-dev" >&2
        exit 1
    fi
done

if [ $# -eq 0 ]; then
  cmake -S . -B build
  cmake --build build --target client
  cmake --build build --target server
  cmake --build build --target test-client
elif [ "$1" = "client" ] || [ "$1" = "server" ] || [ "$1" = "test-client" ]; then
  cmake -S . -B build
  cmake --build build --target "$1"
else
  echo "Usage: $0 [client|server|test-client]"
  exit 1
fi 