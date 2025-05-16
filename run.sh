#!/bin/bash

# 사용법: ./run.sh [client|server] [--port 포트번호]

if [ $# -lt 1 ]; then
  echo "사용법: $0 [client|server] [--port 포트번호]"
  exit 1
fi

TARGET=$1
shift

case $TARGET in
  client)
    EXEC=./build/client/client
    ;;
  server)
    EXEC=./build/server/server
    ;;
  *)
    echo "알 수 없는 대상: $TARGET (client 또는 server만 가능)"
    exit 1
    ;;
esac

if [ ! -x "$EXEC" ]; then
  echo "$EXEC 파일이 존재하지 않거나 실행 권한이 없습니다. 먼저 ./build.sh로 빌드하세요."
  exit 1
fi

$EXEC "$@" 