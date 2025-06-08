// rule.h
#ifndef RULE_H
#define RULE_H

#include <stdbool.h>

#include "piece.h"
#include "types.h"

// 통합 게임 상태 구조체 (클라이언트와 서버 공통 사용)
typedef struct {
    piecestate_t board[BOARD_SIZE][BOARD_SIZE];

    color_t side_to_move;    // TEAM_WHITE 또는 TEAM_BLACK
    int     halfmove_clock;  // 50수 규칙 카운트(폰 이동·캡처 후 0으로)

    // 캐슬링 권리
    bool white_can_castle_kingside;
    bool white_can_castle_queenside;
    bool black_can_castle_kingside;
    bool black_can_castle_queenside;

    // 앙파상 대상 칸 (없으면 -1,-1)
    int en_passant_x, en_passant_y;

    // 게임 상태 (클라이언트에서 사용)
    bool is_check;
    bool is_checkmate;
    bool is_stalemate;
    int  fullmove_number;
} game_t;

// 이동 규칙 검사/적용
bool is_move_legal(const game_t* G, int sx, int sy, int dx, int dy);
void apply_move(game_t* G, int sx, int sy, int dx, int dy);

// 체크, 종료 조건
bool is_in_check(const game_t* G, color_t color);
bool is_checkmate(const game_t* G);
bool is_stalemate(const game_t* G);
bool is_fifty_move_rule(const game_t* G);

#endif  // RULE_H
