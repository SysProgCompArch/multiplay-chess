#!/bin/bash

# 멀티플레이 체스 게임 서버 Docker 실행 스크립트

# 색상 설정
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 이미지 이름 설정
IMAGE_NAME="multiplay-chess-server"
CONTAINER_NAME="chess-server"
PORT=8080

echo -e "${BLUE}🏗️  멀티플레이 체스 서버 Docker 빌드 및 실행${NC}"

# 도움말 출력 함수
show_help() {
    echo "사용법: $0 [옵션]"
    echo ""
    echo "옵션:"
    echo "  build     - Docker 이미지만 빌드"
    echo "  run       - 이미지를 빌드하고 컨테이너 실행"
    echo "  stop      - 실행 중인 컨테이너 중지"
    echo "  logs      - 컨테이너 로그 확인"
    echo "  clean     - 컨테이너와 이미지 삭제"
    echo "  help      - 이 도움말 표시"
    echo ""
    echo "기본 동작 (인자 없음): 이미지 빌드 후 컨테이너 실행"
}

# Docker 이미지 빌드 함수
build_image() {
    echo -e "${YELLOW}🔨 Docker 이미지 빌드 중...${NC}"
    
    # 루트 디렉토리에서 빌드 (전체 프로젝트 컨텍스트 필요)
    cd "$(dirname "$0")/.."
    
    if docker build -f server/Dockerfile -t $IMAGE_NAME .; then
        echo -e "${GREEN}✅ Docker 이미지 빌드 완료!${NC}"
        return 0
    else
        echo -e "${RED}❌ Docker 이미지 빌드 실패!${NC}"
        return 1
    fi
}

# 컨테이너 실행 함수
run_container() {
    echo -e "${YELLOW}🚀 서버 컨테이너 시작 중...${NC}"
    
    # 기존 컨테이너가 실행 중이면 중지
    if docker ps -q -f name=$CONTAINER_NAME | grep -q .; then
        echo -e "${YELLOW}🛑 기존 컨테이너 중지 중...${NC}"
        docker stop $CONTAINER_NAME
        docker rm $CONTAINER_NAME
    fi
    
    # 새 컨테이너 실행
    if docker run -d --name $CONTAINER_NAME -p $PORT:8080 $IMAGE_NAME; then
        echo -e "${GREEN}✅ 서버가 포트 $PORT 에서 실행 중입니다!${NC}"
        echo -e "${BLUE}📋 컨테이너 상태 확인: docker ps${NC}"
        echo -e "${BLUE}📋 로그 확인: docker logs $CONTAINER_NAME${NC}"
        echo -e "${BLUE}📋 컨테이너 중지: docker stop $CONTAINER_NAME${NC}"
        return 0
    else
        echo -e "${RED}❌ 컨테이너 실행 실패!${NC}"
        return 1
    fi
}

# 컨테이너 중지 함수
stop_container() {
    echo -e "${YELLOW}🛑 서버 컨테이너 중지 중...${NC}"
    
    if docker ps -q -f name=$CONTAINER_NAME | grep -q .; then
        docker stop $CONTAINER_NAME
        docker rm $CONTAINER_NAME
        echo -e "${GREEN}✅ 컨테이너가 중지되었습니다.${NC}"
    else
        echo -e "${YELLOW}⚠️  실행 중인 컨테이너가 없습니다.${NC}"
    fi
}

# 로그 확인 함수
show_logs() {
    echo -e "${BLUE}📋 서버 로그 확인 중...${NC}"
    
    if docker ps -q -f name=$CONTAINER_NAME | grep -q .; then
        docker logs -f $CONTAINER_NAME
    else
        echo -e "${RED}❌ 실행 중인 컨테이너가 없습니다.${NC}"
    fi
}

# 정리 함수
clean_up() {
    echo -e "${YELLOW}🧹 컨테이너 및 이미지 정리 중...${NC}"
    
    # 컨테이너 중지 및 삭제
    if docker ps -a -q -f name=$CONTAINER_NAME | grep -q .; then
        docker stop $CONTAINER_NAME 2>/dev/null
        docker rm $CONTAINER_NAME
        echo -e "${GREEN}✅ 컨테이너 삭제 완료${NC}"
    fi
    
    # 이미지 삭제
    if docker images -q $IMAGE_NAME | grep -q .; then
        docker rmi $IMAGE_NAME
        echo -e "${GREEN}✅ 이미지 삭제 완료${NC}"
    fi
}

# 메인 로직
case "$1" in
    "build")
        build_image
        ;;
    "run")
        if build_image; then
            run_container
        fi
        ;;
    "stop")
        stop_container
        ;;
    "logs")
        show_logs
        ;;
    "clean")
        clean_up
        ;;
    "help")
        show_help
        ;;
    "")
        # 기본 동작: 빌드 후 실행
        if build_image; then
            run_container
        fi
        ;;
    *)
        echo -e "${RED}❌ 알 수 없는 옵션: $1${NC}"
        show_help
        exit 1
        ;;
esac 