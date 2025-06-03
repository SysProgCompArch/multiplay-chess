# 서버 아키텍처 가이드

이 문서는 멀티플레이 체스 서버의 코드 구조와 확장 방법을 설명합니다.

## 📁 파일 구조

```
server/
├── main.c                  # 메인 함수, 서버 초기화
├── network.h/c             # 네트워크 관련 (소켓, epoll, 연결 관리)
├── protocol.h/c            # 프로토콜 처리 (메시지 송수신, 프레이밍)
├── handlers/               # 메시지 핸들러들
│   ├── handlers.h          # 핸들러 공통 인터페이스
│   ├── ping_handler.c      # PING 메시지 처리
│   ├── echo_handler.c      # ECHO 메시지 처리
│   └── game_handler.c      # 향후 체스 게임 로직 (예시, 주석 처리)
└── CMakeLists.txt          # 빌드 설정
```

## 🎯 설계 원칙

### 1. 관심사 분리 (Separation of Concerns)
- **Network Layer**: 소켓, epoll, 연결 관리만 담당
- **Protocol Layer**: protobuf 직렬화/역직렬화, 메시지 프레이밍
- **Handler Layer**: 실제 비즈니스 로직 처리

### 2. 확장성 (Extensibility)
- 새로운 메시지 타입을 추가할 때 새 핸들러 파일만 만들면 됨
- 공통 인터페이스(`message_handler_t`)로 일관성 유지

### 3. 테스트 용이성
- 각 모듈이 독립적이므로 단위 테스트 작성 용이
- 핸들러들은 순수 함수로 설계되어 테스트하기 쉬움

## 🚀 새로운 메시지 핸들러 추가하는 방법

### 1단계: 프로토콜 정의 추가
먼저 `common/proto/message.proto`에 새로운 메시지 타입을 추가하세요.

### 2단계: 핸들러 파일 생성
```c
// handlers/my_new_handler.c
#include "../protocol.h"
#include "handlers.h"
#include <stdio.h>

int handle_my_new_message(int fd, ClientMessage *req)
{
    if (req->msg_case != CLIENT_MESSAGE__MSG_MY_NEW_MESSAGE)
    {
        fprintf(stderr, "[fd=%d] Invalid message type\n", fd);
        return -1;
    }
    
    // 비즈니스 로직 구현
    printf("[fd=%d] Handling my new message\n", fd);
    
    // 응답 생성 및 전송
    ServerMessage resp = SERVER_MESSAGE__INIT;
    // resp 설정...
    
    return send_server_message(fd, &resp);
}
```

### 3단계: 헤더에 함수 선언 추가
`handlers/handlers.h`에 함수 선언을 추가하세요:
```c
int handle_my_new_message(int fd, ClientMessage *req);
```

### 4단계: 디스패처에 케이스 추가
`protocol.c`의 `handle_client_message` 함수에 새로운 케이스를 추가하세요:
```c
case CLIENT_MESSAGE__MSG_MY_NEW_MESSAGE:
    result = handle_my_new_message(fd, req);
    break;
```

### 5단계: CMakeLists.txt 업데이트
```cmake
add_executable(server
    main.c
    network.c
    protocol.c
    handlers/ping_handler.c
    handlers/echo_handler.c
    handlers/my_new_handler.c  # 새 핸들러 추가
)
```

## 📋 핸들러 작성 가이드라인

### 반환값 규칙
- `0`: 성공
- `-1`: 에러 (연결이 닫힐 수 있음)

### 에러 처리
- 항상 메시지 타입을 검증하세요
- 메모리 할당 실패를 체크하세요
- 적절한 로그 메시지를 출력하세요

### 메모리 관리
- `send_server_message`가 내부적으로 메모리를 관리합니다
- 핸들러에서 직접 malloc한 메모리는 반드시 해제하세요

## 🔧 유틸리티 함수들

### `send_server_message(int fd, ServerMessage *msg)`
- ServerMessage를 직렬화해서 클라이언트에게 전송
- 길이 프리픽스를 자동으로 추가
- 내부적으로 메모리 관리

### `receive_client_message(int fd)`
- 클라이언트로부터 메시지를 수신하고 파싱
- 길이 프리픽스를 자동으로 처리
- 실패시 NULL 반환

## 📈 성능 고려사항

1. **논블로킹 I/O**: 리스닝 소켓은 논블로킹, 클라이언트 소켓은 블로킹 유지
2. **메모리 효율성**: protobuf 메시지는 사용 후 반드시 해제
3. **확장성**: epoll 기반으로 많은 연결 처리 가능

## 🧪 테스트 방법

```bash
# 서버 빌드
./build.sh server

# 서버 실행
./build/server/server --port 8080

# 다른 터미널에서 클라이언트로 테스트
./build/client/client
```

이 구조를 따르면 코드의 가독성과 유지보수성이 크게 향상되고, 새로운 기능 추가가 매우 쉬워집니다! 