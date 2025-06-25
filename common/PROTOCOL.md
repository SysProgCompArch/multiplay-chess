# 멀티플레이 체스 Protobuf 통신 프로토콜 명세

## 개요

이 프로젝트는 클라이언트와 서버 간 통신에 [Protocol Buffers (protobuf)](https://developers.google.com/protocol-buffers) 바이너리 포맷을 사용합니다. protobuf는 빠르고, 확장성이 뛰어나며, 다양한 언어에서 지원되는 직렬화 프레임워크입니다.

---

## 메시지 구조 (message.proto)

모든 통신 메시지는 protobuf 정의를 따릅니다. 실제 protobuf 정의 파일은 [`common/proto/message.proto`](./proto/message.proto) 입니다.

### 메시지 래퍼 구조

- **클라이언트 → 서버**: 모든 요청은 `ClientMessage` 래퍼로 감싸집니다
- **서버 → 클라이언트**: 모든 응답/브로드캐스트는 `ServerMessage` 래퍼로 감싸집니다

### 메시지 타입 분류

1. **Request**: 클라이언트에서 서버로 보내는 요청 메시지
2. **Response**: 요청에 대한 개별 클라이언트 응답 메시지  
3. **Broadcast**: 요청 유무에 상관없이 서버에서 관련 클라이언트들에게 보내는 알림 메시지

---

## 네트워크 전송 규칙

- **모든 메시지는** `[4바이트 길이 prefix][protobuf 직렬화 메시지]` 형태로 전송합니다.
  - 길이 prefix는 network byte order(빅엔디안, `htonl`/`ntohl`)로 변환된 값입니다.
  - 예: `[0x00 0x00 0x00 0x0A][...10바이트 protobuf...]`

---

## 공통 타입 정의

### 기본 열거형 (Enums)

- `ProtocolVersion`: 프로토콜 버전 관리 (`PV_V1`)
- `PieceType`: 체스 기물 종류 (`PT_PAWN`, `PT_KNIGHT`, `PT_BISHOP`, `PT_ROOK`, `PT_QUEEN`, `PT_KING`)
- `Team`: 팀 색상 (`TEAM_WHITE`, `TEAM_BLACK`)
- `GameEndType`: 게임 종료 유형 (`GAME_END_RESIGN`, `GAME_END_CHECKMATE`, `GAME_END_STALEMATE`, 등)

---

## 클라이언트/서버에서의 사용법

### 1. C 코드에서 protobuf 메시지 생성 및 전송

#### (1) 의존성
- `protobuf-c` 라이브러리 필요 (`sudo apt install protobuf-c-compiler libprotobuf-c-dev`)
- `protoc-c`로 `.proto` 파일을 C 코드로 변환 (CMake에서 자동 처리)
- include: `#include "message.pb-c.h"`

#### (2) 클라이언트 메시지 생성 및 전송 예시 (핑 메시지)
```c
// 1. ClientMessage 래퍼와 PingRequest 초기화
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
PingRequest ping_req = PING_REQUEST__INIT;

// 2. 메시지 내용 설정
ping_req.message = "Hello Server";
client_msg.msg_case = CLIENT_MESSAGE__MSG_PING;
client_msg.ping = &ping_req;
client_msg.version = PROTOCOL_VERSION__PV_V1;

// 3. 직렬화할 크기 계산 및 버퍼 준비
size_t len = client_message__get_packed_size(&client_msg);
uint8_t buf[len];

// 4. protobuf 직렬화
client_message__pack(&client_msg, buf);

// 5. 길이 prefix를 네트워크 바이트 오더로 변환
uint32_t len_net = htonl(len);

// 6. 길이 prefix 및 메시지 전송
send(sockfd, &len_net, 4, 0);
send(sockfd, buf, len, 0);
```

#### (3) 서버 메시지 수신 및 파싱 예시
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

// 4. ServerMessage 파싱
ServerMessage *server_msg = server_message__unpack(NULL, msg_len, buf);
if (!server_msg) {
    // 에러 처리
}

// 5. 메시지 타입에 따라 분기 처리
switch (server_msg->msg_case) {
    case SERVER_MESSAGE__MSG_PING_RES:
        // 핑 응답 처리
        printf("Ping response: %s\n", server_msg->ping_res->message);
        break;
    case SERVER_MESSAGE__MSG_MOVE_BROADCAST:
        // 기물 이동 브로드캐스트 처리
        printf("Move: %s -> %s\n", 
               server_msg->move_broadcast->from, 
               server_msg->move_broadcast->to);
        break;
    case SERVER_MESSAGE__MSG_ERROR:
        // 에러 응답 처리
        printf("Error: %s\n", server_msg->error->message);
        break;
}

// 6. 메시지 해제
server_message__free_unpacked(server_msg, NULL);
```

---

## 주요 메시지 타입별 사용 예시

### 1. 핑/퐁 메시지 (PingRequest/PingResponse)

#### 클라이언트에서 핑 보내기
```c
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
PingRequest ping_req = PING_REQUEST__INIT;

ping_req.message = "ping";
client_msg.msg_case = CLIENT_MESSAGE__MSG_PING;
client_msg.ping = &ping_req;

// 직렬화 및 전송...
```

### 2. 게임 매칭 요청 (MatchGameRequest/MatchGameResponse)

#### 게임 참가 요청
```c
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
MatchGameRequest match_req = MATCH_GAME_REQUEST__INIT;

match_req.player_id = "player_uuid_123";
match_req.desired_game_id = "";  // 새 게임 생성
client_msg.msg_case = CLIENT_MESSAGE__MSG_MATCH_GAME;
client_msg.match_game = &match_req;

// 직렬화 및 전송...
```

### 3. 기물 이동 (MoveRequest/MoveResponse/MoveBroadcast)

#### 기물 이동 요청
```c
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
MoveRequest move_req = MOVE_REQUEST__INIT;

move_req.from = "e2";
move_req.to = "e4";
// move_req.timestamp = ... (현재 시간)
client_msg.msg_case = CLIENT_MESSAGE__MSG_MOVE;
client_msg.move = &move_req;

// 직렬화 및 전송...
```

### 4. 기권 요청 (ResignRequest/ResignResponse/ResignBroadcast)

#### 기권 요청
```c
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
ResignRequest resign_req = RESIGN_REQUEST__INIT;

resign_req.player_id = "player_uuid_123";
client_msg.msg_case = CLIENT_MESSAGE__MSG_RESIGN;
client_msg.resign = &resign_req;

// 직렬화 및 전송...
```

### 5. 채팅 메시지 (ChatRequest/ChatBroadcast)

#### 채팅 메시지 전송
```c
ClientMessage client_msg = CLIENT_MESSAGE__INIT;
ChatRequest chat_req = CHAT_REQUEST__INIT;

chat_req.message = "Good game!";
// chat_req.timestamp = ... (현재 시간)
client_msg.msg_case = CLIENT_MESSAGE__MSG_CHAT;
client_msg.chat = &chat_req;

// 직렬화 및 전송...
```

---

## 메시지 흐름도

```
클라이언트                     서버                     다른 클라이언트들
    |                          |                            |
    |--- PingRequest --------->|                            |
    |<-- PingResponse ---------|                            |
    |                          |                            |
    |--- MatchGameRequest ---->|                            |
    |<-- MatchGameResponse ----|                            |
    |                          |                            |
    |--- MoveRequest --------->|                            |
    |<-- MoveResponse ---------|                            |
    |                          |--- MoveBroadcast --------->|
    |<-- MoveBroadcast --------|                            |
    |                          |                            |
    |--- ResignRequest ------->|                            |
    |<-- ResignResponse -------|                            |
    |                          |--- ResignBroadcast ------->|
    |<-- ResignBroadcast ------|                            |
```

---

## 서버 메시지 타입

### Response 메시지 (개별 클라이언트용)
- `PingResponse`: 핑 요청에 대한 응답
- `EchoResponse`: 에코 요청에 대한 응답
- `MatchGameResponse`: 게임 참가 요청에 대한 응답
- `CancelMatchResponse`: 매칭 취소 요청에 대한 응답
- `MoveResponse`: 기물 이동 요청에 대한 응답
- `ResignResponse`: 기권 요청에 대한 응답
- `ErrorResponse`: 오류 상황에 대한 응답

### Broadcast 메시지 (관련 클라이언트들에게 전송)
- `MoveBroadcast`: 기물 이동 알림 (상대방 포함)
- `CheckBroadcast`: 체크 상황 알림
- `ResignBroadcast`: 기권 알림
- `GameEndBroadcast`: 게임 종료 알림
- `ChatBroadcast`: 채팅 메시지 전달

---

## 타이머 정보

`MoveBroadcast`와 `MatchGameResponse`에는 게임 타이머 관련 정보가 포함됩니다:

- `time_limit_per_player`: 각 플레이어별 제한시간 (초)
- `white_time_remaining`: 백 팀 남은 시간 (초)
- `black_time_remaining`: 흑 팀 남은 시간 (초)
- `game_start_time`: 게임 시작 시간
- `move_timestamp`: 이동 시점 타임스탬프

---

## 게임 종료 처리

게임이 종료되는 경우 다음과 같은 방식으로 처리됩니다:

1. **MoveBroadcast를 통한 즉시 종료**: 이동으로 인해 체크메이트/스테일메이트가 발생한 경우
   - `game_ends = true`
   - `winner_team`: 승리자 색상
   - `end_type`: 종료 유형

2. **별도 브로드캐스트**: 기권, 연결 끊김 등의 경우
   - `ResignBroadcast`: 기권 시
   - `GameEndBroadcast`: 기타 게임 종료 시

---

## 참고/팁

- enum 값은 `PROTOCOL_VERSION__PV_V1`, `TEAM__TEAM_WHITE` 등으로 사용
- 메시지 확장 시 proto 파일만 수정하면 됨
- CMake에서 proto → C 코드 자동 생성됨
- 바이너리 포맷이므로 Wireshark 등으로 패킷을 볼 때는 디코딩 필요
- 모든 메시지는 ClientMessage/ServerMessage 래퍼를 통해 전송
- oneof를 사용하여 하나의 메시지에 여러 타입 중 하나만 포함

---

## FAQ

### Q. JSON 대신 protobuf를 쓰는 이유는?
- 속도가 빠르고, 메시지 크기가 작음
- 타입 안정성, 확장성, 다양한 언어 지원

### Q. 메시지 앞에 4바이트 길이 prefix를 왜 붙이나요?
- TCP는 스트림 기반이므로, 메시지 경계를 명확히 구분하기 위해 필요

### Q. 새로운 메시지 타입을 추가하려면?
- `message.proto`에 새로운 Request/Response/Broadcast를 추가하고
- ClientMessage 또는 ServerMessage의 oneof에 추가한 후
- CMake 빌드만 하면 됨

### Q. Request와 Response, Broadcast의 차이점은?
- **Request**: 클라이언트 → 서버 요청
- **Response**: 요청한 클라이언트에게만 보내는 응답
- **Broadcast**: 관련된 모든 클라이언트에게 보내는 알림

---
