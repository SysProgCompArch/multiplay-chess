#ifndef PIECE_H
#define PIECE_H

#include <stdlib.h>

// 행마 가능 위치를 상대좌표로 표현
typedef struct offset {
    int x;
    int y;
} offset_t;

// 체스 기물 enum
typedef enum piecetype {
    KING,
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT,
    PAWN
} piecetype_t;

// 체스 기물 구조체
typedef struct piece {
    char* name;       // 기물 이름 정의
    piecetype_t  type;       // 기물 종류 정의
    offset_t     offset[56];   // 행마 가능 위치 정의
    int          offset_len; // 행마 가능 위치 배열의 길이
} piece_t;

// 체스 기물 상태 구조체
typedef struct piecestate {
    piece_t* piece;        // 기물 정의를 가치키는 포인터
    int          x;            // x좌표
    int          y;            // y좌표
    short        is_dead;      // 잡힘 여부
    short        is_promoted;  // 프로모션된 상태인지 아닌지 여부

    short    color;         // WHITE(0) or BLACK(1)
    short    is_first_move; // 처음 이동 전이면 1, 이동 후 0
} piecestate_t;

#define WHITE 0
#define BLACK 1

// 전역 기물 테이블 (piece.c 에 정의)
extern piece_t* piece_table[2][6];

// 프로모션 시 기본 퀸 반환 (piece.c 에 정의)
piece_t* get_default_queen(int color);

#endif // PIECE_H
