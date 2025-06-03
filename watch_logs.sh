#!/bin/bash

# 체스 클라이언트 로그 모니터링 스크립트

LOG_FILE="chess_client.log"

# ANSI 색상 코드 정의
RED='\033[0;31m'      # ERROR, FATAL
YELLOW='\033[1;33m'   # WARN
GREEN='\033[0;32m'    # INFO
BLUE='\033[0;34m'     # DEBUG
CYAN='\033[0;36m'     # 타임스탬프
MAGENTA='\033[0;35m'  # 파일명:라인
GRAY='\033[0;37m'     # 함수명
WHITE='\033[1;37m'    # 강조 텍스트
NC='\033[0m'          # No Color (리셋)

# 도움말 출력 함수
show_help() {
    echo -e "${WHITE}사용법:${NC} $0 [옵션]"
    echo -e "${WHITE}옵션:${NC}"
    echo -e "  ${GREEN}--help, -h${NC}     이 도움말을 표시합니다"
    echo -e "  ${GREEN}--error${NC}        ERROR와 FATAL 로그만 표시합니다"
    echo -e "  ${GREEN}--warn${NC}         WARN 이상 로그만 표시합니다"
    echo -e "  ${GREEN}--info${NC}         INFO 이상 로그만 표시합니다"
    echo -e "  ${GREEN}--debug${NC}        모든 로그를 표시합니다 (기본값)"
    echo ""
    echo -e "${WHITE}예시:${NC}"
    echo -e "  $0                 # 모든 로그 표시"
    echo -e "  $0 --error         # 에러 로그만 표시"
    echo -e "  $0 --warn          # 경고 이상 로그만 표시"
    exit 0
}

# 로그 레벨 필터 설정
LOG_FILTER=""
case "$1" in
    --help|-h)
        show_help
        ;;
    --error)
        LOG_FILTER="ERROR\|FATAL"
        FILTER_NAME="${RED}ERROR/FATAL${NC}"
        ;;
    --warn)
        LOG_FILTER="WARN\|ERROR\|FATAL"
        FILTER_NAME="${YELLOW}WARN+${NC}"
        ;;
    --info)
        LOG_FILTER="INFO\|WARN\|ERROR\|FATAL"
        FILTER_NAME="${GREEN}INFO+${NC}"
        ;;
    --debug|"")
        LOG_FILTER=""
        FILTER_NAME="${BLUE}ALL${NC}"
        ;;
    *)
        echo -e "${RED}알 수 없는 옵션: $1${NC}"
        echo -e "도움말을 보려면 ${GREEN}$0 --help${NC}를 사용하세요"
        exit 1
        ;;
esac

echo -e "=== ${CYAN}Chess Client Log Monitor${NC} ==="
echo -e "로그 파일: ${YELLOW}$LOG_FILE${NC}"
echo -e "필터 레벨: $FILTER_NAME"
echo -e "실시간 로그를 보려면 ${RED}Ctrl+C${NC}로 중단하세요"
echo "================================"

# 로그 파일이 존재하는지 확인
if [ ! -f "$LOG_FILE" ]; then
    echo -e "${YELLOW}로그 파일이 아직 생성되지 않았습니다.${NC}"
    echo -e "${GREEN}클라이언트를 실행하면 로그가 생성됩니다.${NC}"
    echo ""
    echo -e "${BLUE}로그 파일 생성을 기다리는 중...${NC}"
    
    # 로그 파일이 생성될 때까지 대기
    while [ ! -f "$LOG_FILE" ]; do
        sleep 1
    done
    
    echo -e "${GREEN}로그 파일이 생성되었습니다!${NC}"
fi

# 실시간 로그 모니터링 with 색상
echo ""
echo -e "=== ${CYAN}실시간 로그${NC} ==="

# tail 명령어 구성
TAIL_CMD="tail -f \"$LOG_FILE\""

# 필터 적용
if [ -n "$LOG_FILTER" ]; then
    TAIL_CMD="$TAIL_CMD | grep \"$LOG_FILTER\""
fi

# 색상 적용
COLOR_CMD="sed -u \
    -e \"s/\(\[[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} [0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\]\)/$(printf "${CYAN}")\1$(printf "${NC}")/g\" \
    -e \"s/\(\[ERROR\]\)/$(printf "${RED}")\1$(printf "${NC}")/g\" \
    -e \"s/\(\[FATAL\]\)/$(printf "${RED}")\1$(printf "${NC}")/g\" \
    -e \"s/\(\[WARN\]\)/$(printf "${YELLOW}")\1$(printf "${NC}")/g\" \
    -e \"s/\(\[INFO\]\)/$(printf "${GREEN}")\1$(printf "${NC}")/g\" \
    -e \"s/\(\[DEBUG\]\)/$(printf "${BLUE}")\1$(printf "${NC}")/g\" \
    -e \"s/\([a-zA-Z_][a-zA-Z0-9_]*\.[ch]:[0-9]\+\)/$(printf "${MAGENTA}")\1$(printf "${NC}")/g\" \
    -e \"s/\([a-zA-Z_][a-zA-Z0-9_]*()[ ]*-\)/$(printf "${GRAY}")\1$(printf "${NC}")/g\""

# 최종 명령어 실행
eval "$TAIL_CMD | $COLOR_CMD" 