#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

// ===================================================================
// 공통 설정 상수
// ===================================================================

// 기본 네트워크 설정
#define DEFAULT_SERVER_HOST        "127.0.0.1"
#define DEFAULT_SERVER_PORT        8080
#define DEFAULT_RECONNECT_INTERVAL 5
#define DEFAULT_CONNECTION_TIMEOUT 10

// 기본 게임 설정
#define DEFAULT_GAME_TIME_LIMIT     600  // 초 단위
#define DEFAULT_MAX_CHAT_MESSAGES   50
#define DEFAULT_CHAT_MESSAGE_LENGTH 256

#endif  // COMMON_CONFIG_H