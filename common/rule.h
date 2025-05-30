// rule.h
#ifndef RULE_H
#define RULE_H

#include <stdbool.h>
#include "piece.h"

typedef struct {
    piecestate_t board[8][8];

    int side_to_move;               // WHITE 또는 BLACK
    int halfmove_clock;             // 50수 규칙 카운트(폰 이동·캡처 후 0으로)
    bool white_can_castle_kingside;
    bool white_can_castle_queenside;
    bool black_can_castle_kingside;
    bool black_can_castle_queenside;
    int en_passant_x, en_passant_y; // 앙파상 대상 칸, 없으면 -1,-1
} game_t;

// 이동 규칙 검사/적용
bool  is_move_legal(const game_t* G, int sx, int sy, int dx, int dy);
void  apply_move(game_t* G, int sx, int sy, int dx, int dy);

// 체크, 종료 조건
bool  is_in_check(const game_t* G, int color);
bool  is_checkmate(const game_t* G);
bool  is_stalemate(const game_t* G);
bool  is_fifty_move_rule(const game_t* G);

#endif // RULE_H
