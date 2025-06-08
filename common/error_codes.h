#ifndef COMMON_ERROR_CODES_H
#define COMMON_ERROR_CODES_H

// ===================================================================
// 공통 에러 코드 정의 (클라이언트-서버 공통 사용)
// ===================================================================

// 네트워크 에러 코드
typedef enum {
    ERROR_NONE = 0,

    // 연결 관련 에러 (1-99)
    ERROR_CONNECTION_FAILED  = 1,
    ERROR_CONNECTION_LOST    = 2,
    ERROR_CONNECTION_TIMEOUT = 3,
    ERROR_INVALID_MESSAGE    = 4,
    ERROR_MESSAGE_TOO_LARGE  = 5,

    // 인증/사용자 관련 에러 (100-199)
    ERROR_INVALID_PLAYER_ID     = 100,
    ERROR_PLAYER_ALREADY_EXISTS = 101,
    ERROR_PLAYER_NOT_FOUND      = 102,
    ERROR_INVALID_USERNAME      = 103,

    // 게임 관련 에러 (200-299)
    ERROR_GAME_NOT_FOUND       = 200,
    ERROR_GAME_FULL            = 201,
    ERROR_GAME_ALREADY_STARTED = 202,
    ERROR_GAME_NOT_STARTED     = 203,
    ERROR_NOT_PLAYER_TURN      = 204,
    ERROR_INVALID_MOVE         = 205,
    ERROR_PIECE_NOT_FOUND      = 206,
    ERROR_INVALID_POSITION     = 207,
    ERROR_GAME_ENDED           = 208,

    // 매칭 관련 에러 (300-399)
    ERROR_MATCHING_FAILED  = 300,
    ERROR_ALREADY_IN_QUEUE = 301,
    ERROR_NOT_IN_QUEUE     = 302,
    ERROR_QUEUE_FULL       = 303,

    // 서버 내부 에러 (500-599)
    ERROR_INTERNAL_SERVER   = 500,
    ERROR_DATABASE_ERROR    = 501,
    ERROR_MEMORY_ALLOCATION = 502,
    ERROR_FILE_IO           = 503,

    // 클라이언트 에러 (600-699)
    ERROR_UI_INITIALIZATION  = 600,
    ERROR_TERMINAL_TOO_SMALL = 601,
    ERROR_INVALID_INPUT      = 602,

    // 기타 에러 (900-999)
    ERROR_UNKNOWN = 999
} error_code_t;

// 에러 코드를 문자열로 변환
const char* error_code_to_string(error_code_t code);

// 에러 메시지 생성 (버퍼 크기는 256바이트 권장)
void format_error_message(error_code_t code, const char* details, char* buffer, size_t buffer_size);

#endif  // COMMON_ERROR_CODES_H