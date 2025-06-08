#ifndef PIECE_H
#define PIECE_H

#include "types.h"

#define MAX_OFFSETS 8

// 체스 기물 구조체
typedef struct piece {
    char        *name;                  // 기물 이름 정의
    piece_type_t type;                  // 기물 종류 정의
    offset_t     offsets[MAX_OFFSETS];  // 행마 가능 위치 정의
    int          offset_len;            // 행마 가능 위치 배열의 길이
} piece_t;

// 체스 기물 상태 구조체
typedef struct {
    piece_t *piece;      // 기물 포인터 (NULL이면 빈 칸)
    team_t   team;       // TEAM_WHITE(0) or TEAM_BLACK(1)
    bool     is_dead;    // 기물이 죽었는지 여부
    bool     has_moved;  // 기물이 이동했는지 여부 (캐슬링, 앙파상 등에 사용)
} piecestate_t;

// 전역 기물 테이블 (piece.c 에 정의)
extern piece_t *piece_table[2][6];

// 프로모션 시 기본 퀸 반환 (piece.c 에 정의)
piece_t *get_default_queen(team_t team);

#endif  // PIECE_H
