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
├── server/                  # 서버 전용 코드
│   ├── CMakeLists.txt
│   └── ... (server source/headers)
│
└── external/                # 외부 라이브러리
    └── cJSON/               # cJSON 라이브러리 소스
```

### 추가 팁

- 공통 코드는 `common/`에 두고, 라이브러리로 빌드해서 클라이언트/서버에서 링크합니다.
- 클라이언트/서버는 각각 독립적인 실행 파일로 빌드합니다.
- 외부 라이브러리가 필요하다면 `external/` 폴더를 만들어 관리할 수 있습니다.
- 필요하다면 `tests/` 폴더를 만들어 테스트 코드도 분리할 수 있습니다.

## 🛠️ 빌드 및 실행 방법

### 1. 전체 빌드 (추천: 자동 빌드)

아래 명령어를 사용하면 build 폴더가 자동으로 생성되고 빌드까지 한 번에 진행됩니다.

```sh
cmake -S . -B build
cmake --build build
```

또는, 제공되는 스크립트를 사용할 수 있습니다:

```sh
./build.sh
```

빌드가 완료되면 `build/client/client`, `build/server/server` 실행 파일이 생성됩니다.

---

### 2. 개별 타겟(클라이언트/서버)만 빌드

- 클라이언트만 빌드:
  ```sh
  cmake --build build --target client
  ```
- 서버만 빌드:
  ```sh
  cmake --build build --target server
  ```

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

### 4. 참고 사항

- common 라이브러리에 변경이 있으면, 클라이언트/서버 빌드시 자동으로 반영됩니다.
- 외부 라이브러리나 테스트 코드가 추가되면, CMake 설정을 확장해 관리할 수 있습니다.
- 기존의 `make` 명령도 사용 가능하지만, 위의 방법을 권장합니다.

---
