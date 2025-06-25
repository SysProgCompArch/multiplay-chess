# 🔧 Chess Client

멀티플레이어 체스 게임의 클라이언트

## 🚀 빌드 및 실행

### 빌드
```bash
# 클라이언트만 빌드
make client

# 또는
./build.sh client
```

### 실행
```bash
# 기본 포트(8080)로 클라이언트 실행
./run.sh client

# 특정 호스트 & 포트로 클라이언트 실행
./run.sh client -h 192.168.1.100 -p 8080
```

## 🏗️ 아키텍처

### 핵심 컴포넌트

2. **client_network.c**: 클라이언트 네트워크 관리
1. **client_state.c**: 클라이언트 상태 관리
2. **game_state.c**: 게임 상태 관리
3. **game_save.c**: 게임 저장
4. **pgn_save.c**: PGN 파일 저장
5. **ui/****: 유저 인터페이스 관리