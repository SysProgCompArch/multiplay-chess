# ♟️ Multiplayer Chess Game (멀티플레이 체스 게임)

2025-1 시스템프로그래밍 001분반 1팀 팀프로젝트

Linux에서 C로 구현된 터미널 기반 멀티플레이어 체스 게임입니다. 컴파일하는 동안 가벼운 체스 게임을 즐기고 싶은 개발자를 위해 설계되었습니다 — GUI 오버헤드 없이 순수한 CLI의 매력을 경험하세요.

## 📌 프로젝트 개요

- **프로젝트명**: TUI 기반 멀티플레이 체스 게임
- **목표**:
  - 시스템 콜과 라이브러리 이해 및 응용
  - 실용적이고 재미있는 시스템 프로그래밍 실습
  - 서버-클라이언트 통신과 멀티스레딩 등 고급 기술 적용
- **사용 기술**: C, pthread, socket, ncurses, JSON, CMake, Docker

## 🎮 주요 기능

### 🧑 사용자 및 매치메이킹

- 닉네임만 입력해 간편 접속 (세션 기반)
- 중앙 서버 기반 자동 매칭
- 연결 끊김 후 재입장 기능

### ♟️ 게임플레이

- TUI 기반 체스 UI (체스판, 기물, 메뉴 등)
- 체스 규칙 완전 구현 (캐슬링, 프로모션, 앙파상, 3수 동형 등)
- PGN 포맷으로 기보 저장 및 리플레이

### 🔗 통신 기능

- 클라이언트 ↔ 서버 실시간 동기화
- 실시간 채팅 기능
- 제한 시간 동기화 (타이머)

### 🛠 기타 기능

- `SIGINT` 무시로 중도 종료 방지
- 마우스 입력 지원 (ncurses 활용)

## ⚙️ 기술 스택

| Area           | Technologies & Tools                                        |
| -------------- | ----------------------------------------------------------- |
| Language       | C                                                           |
| Networking     | Socket API (`socket`, `bind`, `listen`, `accept`, `select`) |
| Multithreading | `pthread`, `mutex`                                          |
| UI             | ncurses, ASCII escape codes                                 |
| Data Format    | JSON                                                        |
| Game Records   | PGN (Portable Game Notation)                                |
| Build & Deploy | CMake, Docker, GitHub Actions                               |

## 🧪 개발 환경

- 운영체제: Linux
- 컴파일러: gcc
- 빌드 도구: CMake
- 패키지 매니저: make
- 협업 도구: GitHub, GitHub Actions (CI/CD), Docker

## 🗂️ 프로젝트 구조 및 CMake 적용

이 프로젝트는 클라이언트, 서버, 그리고 공통 코드를 분리하여 관리하며, CMake를 통해 빌드 시스템을 구성합니다.

```
multiplay-chess/
│
├── CMakeLists.txt           # 최상위 CMake 설정
├── build.sh                 # 빌드 스크립트
├── README.md
│
├── common/                  # 클라이언트와 서버가 공통으로 사용하는 코드
│   ├── CMakeLists.txt
│   └── ... (common source/headers)
│
├── client/                  # 클라이언트 전용 코드
│   ├── CMakeLists.txt
│   └── ... (client source/headers)
│
└── server/                  # 서버 전용 코드
    ├── CMakeLists.txt
    └── ... (server source/headers)
```

### 추가 팁

- 공통 코드는 `common/`에 두고, 라이브러리로 빌드해서 클라이언트/서버에서 링크합니다.
- 클라이언트/서버는 각각 독립적인 실행 파일로 빌드합니다.
- 외부 라이브러리가 필요하다면 `external/` 폴더를 만들어 관리할 수 있습니다.
- 필요하다면 `tests/` 폴더를 만들어 테스트 코드도 분리할 수 있습니다.

## 🛠️ 빌드 및 실행 방법

### 1. 의존성 설치

- Cmake 설치 필요
  ```sh
  sudo apt install cmake
  ```

- 라이브러리 설치 필요
  ```sh
  sudo apt install pkg-config protobuf-compiler protobuf-c-compiler libprotobuf-dev libprotobuf-c-dev libncurses-dev
  ```

### 2. 전체 빌드 및 개별 타겟 빌드 (build.sh 사용)

`build.sh` 스크립트는 인자에 따라 아래와 같이 동작합니다:

- **전체 빌드 (client, server 모두):**
  ```sh
  ./build.sh
  ```
- **클라이언트만 빌드:**
  ```sh
  ./build.sh client
  ```
- **서버만 빌드:**
  ```sh
  ./build.sh server
  ```

빌드가 완료되면 `build/client/client`, `build/server/server` 실행 파일이 생성됩니다.

---

### 3. 실행 방법

- 클라이언트 실행:
  ```sh
  ./client/client
  ```
- 서버 실행:
  ```sh
  ./server/server
  ```
  (위 명령은 build 디렉터리 내에서 실행)

---

### 4. 로그 시스템 사용법

프로젝트는 파일 기반 로그 시스템(클라이언트)과 콘솔 출력 시스템(서버)을 사용합니다.

#### 로그 파일 위치
- **클라이언트 로그**: `chess_client_[PID].log` (PID별 분리, 파일 출력)
- **서버 로그**: 콘솔에 색상과 함께 직접 출력

#### 로그 레벨
- `DEBUG`: 상세한 디버그 정보
- `INFO`: 일반 정보 메시지  
- `WARN`: 경고 메시지
- `ERROR`: 오류 메시지
- `FATAL`: 심각한 오류 메시지

#### 🚀 다중 클라이언트 실행

1. **편리한 다중 클라이언트 실행**:
   ```bash
   # 2개 클라이언트 실행 (기본값)
   ./run_multiple_clients.sh
   
   # 3개 클라이언트 실행
   ./run_multiple_clients.sh 3
   
   # 4개 클라이언트를 2초 간격으로 실행
   ./run_multiple_clients.sh 4 -d 2
   
   # 로그 모니터링 창 없이 실행
   ./run_multiple_clients.sh 2 --no-logs
   
   # 도움말 보기
   ./run_multiple_clients.sh --help
   ```

#### 🔍 실시간 로그 모니터링

1. **클라이언트 로그 모니터링** (권장):
   ```bash
   # 모든 클라이언트 로그를 색상과 함께 표시
   ./watch_logs.sh
   
   # 단일 로그 파일만 모니터링
   ./watch_logs.sh --single
   
   # 에러/경고만 보기
   ./watch_logs.sh --error    # ERROR, FATAL만
   ./watch_logs.sh --warn     # WARN 이상
   ./watch_logs.sh --info     # INFO 이상
   
   # 도움말 보기
   ./watch_logs.sh --help
   ```

2. **서버 로그**: 
   ```bash
   # 서버 실행 시 콘솔에 색상이 있는 로그가 직접 출력됩니다
   ./build/server/server --port 8080
   ```

3. **색상 구분**:
   - 🔴 **빨간색**: ERROR, FATAL 메시지
   - 🟡 **노란색**: WARN 메시지  
   - 🟢 **초록색**: INFO 메시지
   - 🔵 **파란색**: DEBUG 메시지
   - 🔵 **하늘색**: 타임스탬프
   - 🟣 **자주색**: 파일명:라인번호
   - ⚪ **회색**: 함수명

4. **통합 개발 워크플로우**:
   ```bash
   # 개발 시 권장되는 터미널 구성
   
   # 터미널 1: 서버 실행 (로그가 직접 출력됨)
   ./build/server/server --port 8080
   
   # 터미널 2: 클라이언트 로그 모니터링  
   ./watch_logs.sh --info
   
   # 터미널 3: 클라이언트 실행
   ./run_multiple_clients.sh 2
   ```

5. **수동 로그 모니터링**:
   ```bash
   # 특정 클라이언트 로그만 보기
   tail -f chess_client_1234.log
   
   # 모든 클라이언트 로그 동시 보기
   tail -f chess_client_*.log
   
   # 특정 레벨만 필터링
   tail -f chess_client_*.log | grep ERROR
   tail -f chess_client_*.log | grep -E "(WARN|ERROR|FATAL)"
   ```

6. **로그 파일 검색 및 분석**:
   ```bash
   # 클라이언트 네트워크 통신 분석
   grep -i "connect\|disconnect" chess_client_*.log
   
   # 매칭 시스템 분석 (클라이언트 측)
   grep -i "match\|game" chess_client_*.log
   
   # 클라이언트 에러 분석
   grep "ERROR\|FATAL" chess_client_*.log
   
   # 특정 시간대 로그
   grep "2025-01-27 14:" chess_client_*.log
   ```

#### 🛠️ 개발자를 위한 팁

- **테스트 시나리오**: 
  - 서버 먼저 실행 후 여러 클라이언트를 동시에 실행하여 부하 테스트
  - 서버 로그는 콘솔에서 실시간으로 확인 가능
  - 클라이언트 로그는 파일로 저장되어 별도 모니터링 필요

- **로그 분석**:
  - 서버 로그: 실행 터미널에서 직접 확인
  - 클라이언트 로그: `./watch_logs.sh`를 사용하여 모니터링
  - 문제 발생 시 서버와 클라이언트 양쪽 로그를 모두 확인하세요

- **로그 파일 관리**:
  - 클라이언트 로그 파일 정리: `rm chess_client_*.log`
  - 서버 로그는 콘솔 출력이므로 파일 정리가 불필요

## Protocol Buffers (protobuf-c) 사용 안내

Protocol Buffers(protobuf)는 Google에서 개발한 데이터 직렬화 형식입니다. 이 프로젝트에서는 protobuf-c를 사용하여 C 언어로 구현된 클라이언트-서버 간의 통신 메시지를 정의하고 직렬화합니다.

### 주요 특징

- **효율적인 직렬화**: JSON이나 XML보다 더 작은 크기로 데이터를 직렬화할 수 있습니다.
- **타입 안전성**: 컴파일 시점에 메시지 구조를 검증합니다.
- **언어 독립성**: 다양한 프로그래밍 언어에서 사용 가능합니다.
- **자동 코드 생성**: .proto 파일로부터 C 코드를 자동 생성합니다.

### 메시지 스펙 정의

[메시지 스펙 정의 보기](common/PROTOCOL.md)

### 디렉토리 구조

- `common/proto/` : `.proto` 파일 위치
- `common/generated/` : `protoc-c`로 생성된 C 소스/헤더 파일 위치

기본적으로 프로젝트에서 사용하는 메시지 스펙은 `common/proto/message.proto` 파일에 정의되어 있습니다. 이 파일을 수정하여 새로운 메시지 타입을 추가할 수 있습니다.

### .proto 파일 추가 및 컴파일

새 `.proto` 파일을 추가하려면 다음 단계를 따르세요:

1. `.proto` 파일을 `common/proto/`에 추가
2. 아래 명령어로 C 파일 생성 (`protobuf-c` 설치 필요, 하단 의존성 섹션 참고)

```bash
protoc-c -I common/proto --c_out=common/generated/ common/proto/message.proto
```

또는 빌드 스크립트를 실행하면 자동으로 컴파일됩니다.
