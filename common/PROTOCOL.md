# 멀티플레이 체스 Protobuf 통신 프로토콜 명세

## 개요

이 프로젝트는 클라이언트와 서버 간 통신에 [Protocol Buffers (protobuf)](https://developers.google.com/protocol-buffers) 바이너리 포맷을 사용합니다. protobuf는 빠르고, 확장성이 뛰어나며, 다양한 언어에서 지원되는 직렬화 프레임워크입니다.

---

## 메시지 구조 (message.proto)

모든 통신 메시지는 아래와 같은 protobuf 정의를 따릅니다.

```proto
syntax = "proto3";

enum OpCode {
  PING = 0;   // 핑/퐁(하트비트)
  CHAT = 1;   // 채팅 메시지
  MOVE = 2;   // 기물 옮김
  ... // 추가 가능
}

message ChatData {
  string msg = 1; // 채팅 메시지 내용
}

message MoveData {
  string target = 1; // 옮길 대상 기물 위치 (예: "a2")
  string dest = 2;   // 옮길 새 위치 (예: "a3")
}

message Message {
  OpCode op = 1;
  oneof data {
    MoveData move = 2;
    ChatData chat = 3;
    // 다른 데이터 구조체 추가 가능
  }
}

...
```

> 실제 파일 위치: `common/proto/message.proto`

---

## 네트워크 전송 규칙

- **모든 메시지는** `[4바이트 길이 prefix][protobuf 직렬화 메시지]` 형태로 전송합니다.
  - 길이 prefix는 network byte order(빅엔디안, `htonl`/`ntohl`)로 변환된 값입니다.
  - 예: `[0x00 0x00 0x00 0x0A][...10바이트 protobuf...]`

---

## 클라이언트/서버에서의 사용법

### 1. C 코드에서 protobuf 메시지 생성 및 전송

#### (1) 의존성
- `protobuf-c` 라이브러리 필요 (`sudo apt install protobuf-c-compiler libprotobuf-c-dev`)
- `protoc-c`로 `.proto` 파일을 C 코드로 변환 (CMake에서 자동 처리)
- include: `#include "message.pb-c.h"`

#### (2) 메시지 생성 및 직렬화/전송 예시 (핑 메시지)
```c
// 1. 메시지 구조체 초기화 및 값 설정
Message ping = MESSAGE__INIT;
ping.op = OP_CODE__PING;

// 2. 직렬화할 크기 계산 및 버퍼 준비
size_t len = message__get_packed_size(&ping);
uint8_t buf[len];

// 3. protobuf 직렬화
message__pack(&ping, buf);

// 4. 길이 prefix를 네트워크 바이트 오더로 변환
uint32_t len_net = htonl(len);

// 5. 길이 prefix 전송
send(sockfd, &len_net, 4, 0);

// 6. 직렬화된 메시지 전송
send(sockfd, buf, len, 0);
```

#### (3) 메시지 수신 및 파싱 예시
```c
// 1. 길이 prefix(4바이트) 수신
uint8_t lenbuf[4];
if (recv(sockfd, lenbuf, 4, MSG_WAITALL) != 4) {
    // 에러 처리
}

// 2. prefix를 호스트 바이트 오더로 변환
uint32_t msg_len;
memcpy(&msg_len, lenbuf, 4);
msg_len = ntohl(msg_len);

// 3. 메시지 본문 수신
uint8_t buf[msg_len];
if (recv(sockfd, buf, msg_len, MSG_WAITALL) != msg_len) {
    // 에러 처리
}

// 4. protobuf 파싱
Message *msg = message__unpack(NULL, msg_len, buf);
if (!msg) {
    // 에러 처리
}

// 5. 메시지 타입에 따라 분기 처리
if (msg->op == OP_CODE__PING) {
    // 핑/퐁 처리
}

// 6. 메시지 해제
message__free_unpacked(msg, NULL);
```

---

## 예시: 핑/퐁 메시지 송수신 (클라이언트/서버 공통)

### 핑 메시지 보내기
```c
// 1. 핑 메시지 구조체 초기화
Message ping = MESSAGE__INIT;
ping.op = OP_CODE__PING;

// 2. 직렬화 및 전송
size_t len = message__get_packed_size(&ping);
uint8_t buf[len];
message__pack(&ping, buf);
uint32_t len_net = htonl(len);
send(sockfd, &len_net, 4, 0);
send(sockfd, buf, len, 0);
```

### 메시지 수신 및 파싱
```c
// 1. 길이 prefix 수신
uint8_t lenbuf[4];
if (recv(sockfd, lenbuf, 4, MSG_WAITALL) != 4) {
    // 에러 처리
}

// 2. prefix 변환
uint32_t msg_len;
memcpy(&msg_len, lenbuf, 4);
msg_len = ntohl(msg_len);

// 3. 메시지 본문 수신
uint8_t buf[msg_len];
if (recv(sockfd, buf, msg_len, MSG_WAITALL) != msg_len) {
    // 에러 처리
}

// 4. protobuf 파싱
Message *msg = message__unpack(NULL, msg_len, buf);
if (!msg) {
    // 에러 처리
}

// 5. 메시지 타입 분기
if (msg->op == OP_CODE__PING) {
    // 핑/퐁 처리
}

// 6. 메시지 해제
message__free_unpacked(msg, NULL);
```

---

## 예시: 기물 옮김(MOVE) 메시지 보내기
```c
// 1. 메시지 및 데이터 구조체 초기화
Message move_msg = MESSAGE__INIT;
move_msg.op = OP_CODE__MOVE;
MoveData move = MOVE_DATA__INIT;
move.target = "a2";
move.dest = "a3";
move_msg.move = &move;

// 2. 직렬화 및 전송
size_t len = message__get_packed_size(&move_msg);
uint8_t buf[len];
message__pack(&move_msg, buf);
uint32_t len_net = htonl(len);
send(sockfd, &len_net, 4, 0);
send(sockfd, buf, len, 0);
```

---

## 참고/팁
- enum 값은 `OP_CODE__PING`, `OP_CODE__MOVE` 등으로 사용 (별칭을 써도 됨)
- 메시지 확장 시 proto 파일만 수정하면 됨
- CMake에서 proto → C 코드 자동 생성됨
- 바이너리 포맷이므로 Wireshark 등으로 패킷을 볼 때는 디코딩 필요

---

## FAQ

### Q. JSON 대신 protobuf를 쓰는 이유는?
- 속도가 빠르고, 메시지 크기가 작음
- 타입 안정성, 확장성, 다양한 언어 지원

### Q. 메시지 앞에 4바이트 길이 prefix를 왜 붙이나요?
- TCP는 스트림 기반이므로, 메시지 경계를 명확히 구분하기 위해 필요

### Q. 새로운 메시지 타입을 추가하려면?
- `message.proto`에 새로운 op/데이터 구조체를 추가하고, CMake 빌드만 하면 됨

---

궁금한 점은 언제든 README나 팀원에게 문의하세요!
