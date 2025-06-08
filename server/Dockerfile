# 멀티플레이 체스 게임 서버를 위한 Dockerfile
FROM ubuntu:22.04

# 환경 변수 설정 (패키지 설치 시 인터랙티브 모드를 비활성화)
ENV DEBIAN_FRONTEND=noninteractive

# 시스템 업데이트 및 필요한 패키지 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    protobuf-compiler \
    protobuf-c-compiler \
    libprotobuf-dev \
    libprotobuf-c-dev \
    libncurses-dev \
    libncursesw5-dev \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 프로젝트 파일 복사
COPY . .

# 빌드 디렉토리 생성 및 CMake 설정
RUN mkdir -p build

# CMake를 사용하여 서버만 빌드
RUN cd build && \
    cmake .. && \
    cmake --build . --target server

# 서버 실행 포트 노출 (기본 체스 서버 포트)
EXPOSE 8080

# 빌드된 서버 실행 파일 위치로 작업 디렉토리 변경
WORKDIR /app/build

# 서버 실행 명령
CMD ["./server/server"]

# 컨테이너 실행 시 로그 확인을 위한 레이블 추가
LABEL description="Multiplayer Chess Game Server"
LABEL version="1.0"
LABEL maintainer="multiplay-chess-team" 