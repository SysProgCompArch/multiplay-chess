# 🔧 Chess Server

Linux에서 C로 구현된 멀티플레이어 체스 게임의 서버 컴포넌트입니다.

## 🚀 빌드 및 실행

### 빌드
```bash
# 서버만 빌드
./build.sh server

# 전체 빌드
./build.sh
```

### 실행
```bash
# 기본 포트(8080)로 서버 실행
./build/server/server

# 특정 포트로 서버 실행
./build/server/server --port 9090
```

## 📋 로그 시스템

서버는 콘솔에 색상이 있는 로그를 직접 출력합니다. 별도의 로그 파일을 생성하지 않으며, 실시간으로 서버 상태를 확인할 수 있습니다.

### 로그 출력 특징
- **실시간 콘솔 출력**: 서버 실행 시 터미널에 바로 표시
- **색상 구분**: 로그 레벨별로 다른 색상 적용
- **상세한 정보**: 타임스탬프, 파일명:라인번호, 함수명 포함

### 로그 레벨
- `DEBUG`: 상세한 디버그 정보 (네트워크 이벤트, 메시지 처리 등)
- `INFO`: 일반 정보 메시지 (클라이언트 연결, 게임 시작 등)
- `WARN`: 경고 메시지 (매칭 실패, 잘못된 요청 등)
- `ERROR`: 오류 메시지 (네트워크 오류, 처리 실패 등)
- `FATAL`: 심각한 오류 메시지 (서버 종료 필요한 오류)

### 🎨 색상 구분

- 🔴 **빨간색**: ERROR, FATAL 메시지
- 🟡 **노란색**: WARN 메시지  
- 🟢 **초록색**: INFO 메시지
- 🔵 **파란색**: DEBUG 메시지
- 🔵 **하늘색**: 타임스탬프
- 🟣 **자주색**: 파일명:라인번호
- ⚪ **회색**: 함수명

### 로그 출력 예시

```
[2025-06-03 20:16:58] [INFO] main.c:41 main() - Chess server starting... (PID: 159598)
[2025-06-03 20:16:58] [DEBUG] server_network.c:55 create_and_bind_listener() - Socket created: fd=3
[2025-06-03 20:16:58] [INFO] server_network.c:161 event_loop() - Starting event loop...
[2025-06-03 20:17:20] [INFO] server_network.c:156 handle_new_connection() - New client connected: fd=7
[2025-06-03 20:17:21] [INFO] ping.c:18 handle_ping_message() - Ping received from fd=7: Hello Server!
[2025-06-03 20:17:25] [INFO] match_manager.c:178 add_player_to_matching() - Player Alice(fd=7) added to waiting queue
```

## 🏗️ 아키텍처

### 핵심 컴포넌트

1. **server_network.c**: epoll 기반 네트워크 이벤트 처리
2. **protocol.c**: protobuf 메시지 직렬화/역직렬화
3. **match_manager.c**: 매칭 시스템 및 게임 관리
4. **handlers/**: 메시지 타입별 처리 핸들러들
5. **common/logger.c**: 스레드 안전 로그 시스템 (콘솔 출력)

## 🛠️ 개발자 가이드

### 실시간 로그 모니터링

서버 실행 시 터미널에서 실시간으로 로그를 확인할 수 있습니다:

```bash
# 서버 실행 - 로그가 실시간으로 콘솔에 출력됨
./build/server/server --port 8080
```

### 디버깅 팁

1. **서버 시작 시 로그 확인**:
   ```bash
   # 서버 실행하면 초기화 과정이 로그로 표시됨
   ./build/server/server
   ```

2. **연결 문제 진단**:
   - 클라이언트 연결 시 `New client connected` 메시지 확인
   - 네트워크 에러 시 빨간색 ERROR 메시지 확인

3. **매칭 시스템 디버깅**:
   - 매칭 요청 시 `Match request from player` 메시지 확인
   - 게임 시작 시 `Match found! Game started` 메시지 확인

### 성능 모니터링

```bash
# 동시 연결 수 확인
netstat -an | grep :8080 | grep ESTABLISHED | wc -l

# 서버 프로세스 리소스 사용량
ps aux | grep server
```

## 🐛 문제 해결

### 일반적인 문제들

1. **포트 이미 사용 중**:
   ```bash
   # 포트 사용 프로세스 확인
   lsof -i :8080
   
   # 다른 포트로 실행
   ./build/server/server --port 9090
   ```

2. **메모리 누수 디버깅**:
   ```bash
   # valgrind로 메모리 검사
   valgrind --leak-check=full ./build/server/server
   ```

### 로그를 통한 문제 진단

서버 실행 중 터미널에서 다음 항목들을 확인하세요:

- **초기화 과정**: 서버 시작 시 나타나는 INFO 메시지들
- **클라이언트 연결**: `New client connected` 메시지
- **매칭 시스템**: 매칭 요청과 게임 생성 관련 메시지
- **에러 메시지**: 빨간색으로 표시되는 ERROR/FATAL 메시지

### 로그 출력 리다이렉션

필요한 경우 로그를 파일로 저장할 수 있습니다:

```bash
# 모든 출력을 파일로 저장
./build/server/server 2>&1 | tee server_output.log

# 에러만 파일로 저장
./build/server/server 2> server_errors.log
``` 