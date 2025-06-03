#!/bin/bash

# 체스 클라이언트 로그 모니터링 스크립트

LOG_FILE="chess_client.log"

echo "=== Chess Client Log Monitor ==="
echo "로그 파일: $LOG_FILE"
echo "실시간 로그를 보려면 Ctrl+C로 중단하세요"
echo "================================"

# 로그 파일이 존재하는지 확인
if [ ! -f "$LOG_FILE" ]; then
    echo "로그 파일이 아직 생성되지 않았습니다."
    echo "클라이언트를 실행하면 로그가 생성됩니다."
    echo ""
    echo "로그 파일 생성을 기다리는 중..."
    
    # 로그 파일이 생성될 때까지 대기
    while [ ! -f "$LOG_FILE" ]; do
        sleep 1
    done
    
    echo "로그 파일이 생성되었습니다!"
fi

# 실시간 로그 모니터링
echo ""
echo "=== 실시간 로그 ==="
tail -f "$LOG_FILE" 