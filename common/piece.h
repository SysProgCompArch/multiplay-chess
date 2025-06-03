#ifndef PIECE_H
#define PIECE_H

#include "common.h"

#define WHITE 0
#define BLACK 1

// 전역 기물 테이블 (piece.c 에 정의)
extern piece_t *piece_table[2][6];

// 프로모션 시 기본 퀸 반환 (piece.c 에 정의)
piece_t *get_default_queen(int color);

#endif // PIECE_H
