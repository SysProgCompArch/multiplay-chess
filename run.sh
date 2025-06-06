#!/bin/bash

# 사용법: ./run.sh [client|server|test-client] [옵션들...]

if [ $# -lt 1 ]; then
  echo "사용법: $0 [client|server|test-client] [옵션들...]"
  echo "  client: 일반 클라이언트 실행 [-h HOST] [-p PORT]"
  echo "  server: 서버 실행 [--port 포트번호]"
  echo "  test-client: 테스트 클라이언트 실행 [서버IP] [포트번호]"
  echo ""
  echo "클라이언트 옵션:"
  echo "  -h, --host HOST    서버 호스트 (기본값: 127.0.0.1)"
  echo "  -p, --port PORT    서버 포트 (기본값: 8080)"
  echo "  --help             도움말 출력"
  echo ""
  echo "예시:"
  echo "  $0 client                    # 기본값으로 클라이언트 실행"
  echo "  $0 client -h 192.168.1.100  # 특정 호스트로 연결"
  echo "  $0 client -p 9090           # 특정 포트로 연결"
  echo "  $0 client -h 192.168.1.100 -p 9090  # 호스트와 포트 모두 지정"
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
  test-client)
    EXEC=./build/client/test-client
    ;;
  *)
    echo "알 수 없는 대상: $TARGET (client, server, test-client 중 하나)"
    exit 1
    ;;
esac

if [ ! -x "$EXEC" ]; then
  echo "$EXEC 파일이 존재하지 않거나 실행 권한이 없습니다. 먼저 ./build.sh로 빌드하세요."
  exit 1
fi

$EXEC "$@" 