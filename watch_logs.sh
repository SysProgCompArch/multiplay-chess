#!/bin/bash

# 체스 클라이언트 로그 모니터링 스크립트 (다중 클라이언트 지원)

LOG_PATTERN="chess_client*.log"
LOG_FILE="chess_client.log"  # 기본값 (호환성)

# ANSI 색상 코드 정의
RED='\033[0;31m'      # ERROR, FATAL
YELLOW='\033[1;33m'   # WARN
GREEN='\033[0;32m'    # INFO
BLUE='\033[0;34m'     # DEBUG
CYAN='\033[0;36m'     # 타임스탬프
MAGENTA='\033[0;35m'  # 파일명:라인
GRAY='\033[0;37m'     # 함수명
WHITE='\033[1;37m'    # 강조 텍스트
ORANGE='\033[0;33m'   # PID 표시
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
    echo -e "  ${GREEN}--single${NC}       단일 로그 파일만 모니터링 (chess_client.log)"
    echo -e "  ${GREEN}--all${NC}          모든 클라이언트 로그 파일을 모니터링 (기본값)"
    echo ""
    echo -e "${WHITE}예시:${NC}"
    echo -e "  $0                 # 모든 클라이언트 로그 표시"
    echo -e "  $0 --single        # 단일 로그 파일만 표시"
    echo -e "  $0 --error         # 모든 클라이언트의 에러 로그만 표시"
    echo -e "  $0 --warn --single # 단일 파일의 경고 이상 로그만 표시"
    exit 0
}

# 로그 레벨 필터 설정
LOG_FILTER=""
MONITOR_MODE="all"  # all 또는 single

while [[ $# -gt 0 ]]; do
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
        --debug)
            LOG_FILTER=""
            FILTER_NAME="${BLUE}ALL${NC}"
            ;;
        --single)
            MONITOR_MODE="single"
            ;;
        --all)
            MONITOR_MODE="all"
            ;;
        *)
            echo -e "${RED}알 수 없는 옵션: $1${NC}"
            echo -e "도움말을 보려면 ${GREEN}$0 --help${NC}를 사용하세요"
            exit 1
            ;;
    esac
    shift
done

# 기본값 설정
if [ -z "$FILTER_NAME" ]; then
    FILTER_NAME="${BLUE}ALL${NC}"
fi

echo -e "=== ${CYAN}Chess Client Log Monitor${NC} ==="

if [ "$MONITOR_MODE" = "single" ]; then
    echo -e "모니터링 모드: ${YELLOW}단일 파일${NC} ($LOG_FILE)"
    LOG_FILES=("$LOG_FILE")
else
    echo -e "모니터링 모드: ${GREEN}다중 클라이언트${NC} ($LOG_PATTERN)"
    # 존재하는 로그 파일들 찾기
    LOG_FILES=($(ls chess_client*.log 2>/dev/null))
    
    if [ ${#LOG_FILES[@]} -eq 0 ]; then
        LOG_FILES=("$LOG_FILE")  # 기본 파일로 대체
    fi
fi

echo -e "필터 레벨: $FILTER_NAME"
echo -e "실시간 로그를 보려면 ${RED}Ctrl+C${NC}로 중단하세요"
echo "================================"

# 로그 파일 존재 확인 및 대기
any_file_exists=false
for log_file in "${LOG_FILES[@]}"; do
    if [ -f "$log_file" ]; then
        any_file_exists=true
        break
    fi
done

if [ "$any_file_exists" = false ]; then
    echo -e "${YELLOW}로그 파일이 아직 생성되지 않았습니다.${NC}"
    echo -e "${GREEN}클라이언트를 실행하면 로그가 생성됩니다.${NC}"
    echo ""
    echo -e "${BLUE}로그 파일 생성을 기다리는 중...${NC}"
    
    # 로그 파일이 생성될 때까지 대기
    while [ "$any_file_exists" = false ]; do
        sleep 1
        if [ "$MONITOR_MODE" = "all" ]; then
            LOG_FILES=($(ls chess_client*.log 2>/dev/null))
        fi
        
        for log_file in "${LOG_FILES[@]}"; do
            if [ -f "$log_file" ]; then
                any_file_exists=true
                break
            fi
        done
    done
    
    echo -e "${GREEN}로그 파일이 생성되었습니다!${NC}"
fi

# 현재 모니터링할 파일들 표시
echo ""
echo -e "${CYAN}모니터링 중인 파일들:${NC}"
for log_file in "${LOG_FILES[@]}"; do
    if [ -f "$log_file" ]; then
        # PID 추출
        if [[ "$log_file" =~ chess_client_([0-9]+)\.log ]]; then
            pid="${BASH_REMATCH[1]}"
            echo -e "  ${GREEN}✓${NC} $log_file ${ORANGE}(PID: $pid)${NC}"
        else
            echo -e "  ${GREEN}✓${NC} $log_file"
        fi
    else
        echo -e "  ${GRAY}✗${NC} $log_file ${GRAY}(대기 중)${NC}"
    fi
done

# 실시간 로그 모니터링 with 색상
echo ""
echo -e "=== ${CYAN}실시간 로그${NC} ==="

# tail 명령어 구성 (여러 파일 동시 모니터링)
if [ ${#LOG_FILES[@]} -eq 1 ]; then
    # 단일 파일
    TAIL_CMD="tail -f \"${LOG_FILES[0]}\" 2>/dev/null"
else
    # 여러 파일 (파일명 표시 포함)
    TAIL_CMD="tail -f ${LOG_FILES[*]} 2>/dev/null"
fi

# 필터 적용
if [ -n "$LOG_FILTER" ]; then
    TAIL_CMD="$TAIL_CMD | grep \"$LOG_FILTER\""
fi

# 색상 적용 (PID 정보도 색상 적용)
COLOR_CMD="sed -u \
    -e \"s/^==> \(chess_client_\([0-9]\+\)\.log\) <==/$(printf "${WHITE}${ORANGE}")● Client PID:\2$(printf "${NC}") ────────────────/g\" \
    -e \"s/^==> \(chess_client\.log\) <==/$(printf "${WHITE}${ORANGE}")● Default Client$(printf "${NC}") ────────────────/g\" \
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