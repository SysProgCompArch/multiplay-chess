#!/bin/sh

if [ $# -eq 0 ]; then
  cmake -S . -B build
  cmake --build build --target client
  cmake --build build --target server
elif [ "$1" = "client" ] || [ "$1" = "server" ]; then
  cmake -S . -B build
  cmake --build build --target "$1"
else
  echo "Usage: $0 [client|server]"
  exit 1
fi 