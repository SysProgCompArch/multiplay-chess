# 🔧 Chess Server

멀티플레이어 체스 게임의 서버

## 🚀 빌드 및 실행

### 빌드
```bash
# 서버만 빌드
make server

# 또는
./build.sh server
```

### 실행
```bash
# 기본 포트(8080)로 서버 실행
./run.sh server

# 특정 포트로 서버 실행
./run.sh server -p 8081
```

## 🏗️ 아키텍처

### 핵심 컴포넌트

1. **server_network.c**: epoll 기반 네트워크 이벤트 처리
2. **match_manager.c**: 매칭 시스템 및 게임 관리
3. **handlers/**: 메시지 타입별 처리 핸들러들