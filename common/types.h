#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdbool.h>
#include <time.h>

// 보드 크기
#define BOARD_SIZE 8

// 체스 기물 타입 (공통)
typedef enum {
    PIECE_PAWN   = 0,
    PIECE_KNIGHT = 1,
    PIECE_BISHOP = 2,
    PIECE_ROOK   = 3,
    PIECE_QUEEN  = 4,
    PIECE_KING   = 5
} piece_type_t;

// 체스 기물 색상 (팀)
typedef enum {
    TEAM_WHITE = 0,
    TEAM_BLACK = 1
} team_t;

// 이전 버전과의 호환성을 위한 매크로 (새 코드에서는 TEAM_WHITE/TEAM_BLACK 사용)
#define WHITE TEAM_WHITE
#define BLACK TEAM_BLACK

// 기물 이동 오프셋
typedef struct {
    int x;
    int y;
} offset_t;

// 채팅 메시지 관련 상수
#define MAX_CHAT_MESSAGES  50
#define MAX_MESSAGE_LENGTH 256

// 플레이어 이름 관련 상수
#define MAX_PLAYER_NAME_LENGTH 32

#endif  // COMMON_TYPES_H