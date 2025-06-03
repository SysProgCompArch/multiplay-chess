#!/bin/bash

# 다중 클라이언트 실행 도우미 스크립트

CLIENT_EXEC="./build/client/client"
DEFAULT_COUNT=2
DELAY=1  # 클라이언트 간 실행 지연 시간 (초)

# 색상 정의
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

show_help() {
    echo -e "${GREEN}사용법:${NC} $0 [클라이언트 수] [옵션]"
    echo ""
    echo -e "${GREEN}인수:${NC}"
    echo -e "  클라이언트 수    실행할 클라이언트 개수 (기본값: $DEFAULT_COUNT)"
    echo ""
    echo -e "${GREEN}옵션:${NC}"
    echo -e "  -h, --help       이 도움말 표시"
    echo -e "  -d, --delay N    클라이언트 간 실행 지연 시간 (초, 기본값: $DELAY)"
    echo -e "  --no-logs        로그 모니터링 창을 자동으로 열지 않음"
    echo ""
    echo -e "${GREEN}예시:${NC}"
    echo -e "  $0               # 2개 클라이언트 실행"
    echo -e "  $0 3             # 3개 클라이언트 실행"
    echo -e "  $0 4 -d 2        # 4개 클라이언트를 2초 간격으로 실행"
    echo -e "  $0 2 --no-logs   # 2개 클라이언트 실행 (로그 모니터링 없음)"
    echo ""
    echo -e "${BLUE}참고:${NC}"
    echo -e "  - 각 클라이언트는 별도의 터미널에서 실행됩니다"
    echo -e "  - 로그 파일은 chess_client_[PID].log 형식으로 생성됩니다"
    echo -e "  - 로그 모니터링은 ./watch_logs.sh로 확인할 수 있습니다"
    exit 0
}

# 클라이언트 실행 파일 확인
check_client_executable() {
    if [ ! -f "$CLIENT_EXEC" ]; then
        echo -e "${RED}오류:${NC} 클라이언트 실행 파일을 찾을 수 없습니다: $CLIENT_EXEC"
        echo -e "${YELLOW}힌트:${NC} ./build.sh client 명령으로 먼저 빌드하세요"
        exit 1
    fi

    if [ ! -x "$CLIENT_EXEC" ]; then
        echo -e "${RED}오류:${NC} 클라이언트 실행 파일에 실행 권한이 없습니다: $CLIENT_EXEC"
        exit 1
    fi
}

# 터미널에서 클라이언트 실행
run_client() {
    local client_num=$1
    local title="Chess Client #$client_num"
    
    # 다양한 터미널 에뮬레이터 지원
    if command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="$title" -- bash -c "$CLIENT_EXEC; echo 'Press Enter to close...'; read"
    elif command -v xterm &> /dev/null; then
        xterm -title "$title" -e bash -c "$CLIENT_EXEC; echo 'Press Enter to close...'; read" &
    elif command -v konsole &> /dev/null; then
        konsole --title "$title" -e bash -c "$CLIENT_EXEC; echo 'Press Enter to close...'; read" &
    elif command -v terminator &> /dev/null; then
        terminator --title="$title" -x bash -c "$CLIENT_EXEC; echo 'Press Enter to close...'; read" &
    else
        echo -e "${RED}오류:${NC} 지원되는 터미널 에뮬레이터를 찾을 수 없습니다"
        echo -e "${YELLOW}지원되는 터미널:${NC} gnome-terminal, xterm, konsole, terminator"
        exit 1
    fi
}

# 로그 모니터링 창 열기
open_log_monitor() {
    if command -v gnome-terminal &> /dev/null; then
        gnome-terminal --title="Chess Logs Monitor" -- bash -c "./watch_logs.sh; bash"
    elif command -v xterm &> /dev/null; then
        xterm -title "Chess Logs Monitor" -e bash -c "./watch_logs.sh; bash" &
    elif command -v konsole &> /dev/null; then
        konsole --title "Chess Logs Monitor" -e bash -c "./watch_logs.sh; bash" &
    elif command -v terminator &> /dev/null; then
        terminator --title="Chess Logs Monitor" -x bash -c "./watch_logs.sh; bash" &
    fi
}

# 인수 파싱
CLIENT_COUNT=$DEFAULT_COUNT
OPEN_LOGS=true

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        -d|--delay)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                DELAY=$2
                shift 2
            else
                echo -e "${RED}오류:${NC} --delay 옵션에는 숫자를 입력해야 합니다"
                exit 1
            fi
            ;;
        --no-logs)
            OPEN_LOGS=false
            shift
            ;;
        *)
            if [[ $1 =~ ^[0-9]+$ ]]; then
                CLIENT_COUNT=$1
                shift
            else
                echo -e "${RED}오류:${NC} 알 수 없는 옵션: $1"
                echo -e "도움말을 보려면 ${GREEN}$0 --help${NC}를 사용하세요"
                exit 1
            fi
            ;;
    esac
done

# 유효성 검사
if [ $CLIENT_COUNT -lt 1 ] || [ $CLIENT_COUNT -gt 10 ]; then
    echo -e "${RED}오류:${NC} 클라이언트 수는 1-10 범위여야 합니다"
    exit 1
fi

echo -e "=== ${BLUE}다중 클라이언트 실행${NC} ==="
echo -e "클라이언트 수: ${GREEN}$CLIENT_COUNT${NC}"
echo -e "실행 지연: ${GREEN}${DELAY}초${NC}"
echo -e "로그 모니터링: ${GREEN}$([ $OPEN_LOGS = true ] && echo '활성화' || echo '비활성화')${NC}"
echo ""

# 클라이언트 실행 파일 확인
check_client_executable

# 기존 로그 파일 정리 (선택사항)
echo -e "${YELLOW}기존 로그 파일을 정리하시겠습니까? (y/N):${NC} "
read -r answer
if [[ $answer =~ ^[Yy]$ ]]; then
    rm -f chess_client*.log
    echo -e "${GREEN}기존 로그 파일을 정리했습니다${NC}"
fi

# 클라이언트들 실행
echo -e "${BLUE}클라이언트들을 실행하고 있습니다...${NC}"
for i in $(seq 1 $CLIENT_COUNT); do
    echo -e "  ${GREEN}✓${NC} 클라이언트 #$i 실행"
    run_client $i
    
    if [ $i -lt $CLIENT_COUNT ]; then
        sleep $DELAY
    fi
done

# 잠시 대기 후 로그 모니터링 창 열기
if [ $OPEN_LOGS = true ]; then
    echo ""
    echo -e "${BLUE}로그 모니터링 창을 여는 중...${NC}"
    sleep 2
    open_log_monitor
fi

echo ""
echo -e "${GREEN}완료!${NC} $CLIENT_COUNT개의 클라이언트가 실행되었습니다"
echo -e "${YELLOW}팁:${NC}"
echo -e "  - 로그 확인: ${BLUE}./watch_logs.sh${NC}"
echo -e "  - 에러만 보기: ${BLUE}./watch_logs.sh --error${NC}"
echo -e "  - 단일 파일만: ${BLUE}./watch_logs.sh --single${NC}" 