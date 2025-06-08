// piece.c
#include "piece.h"

// --- 화이트 기물들 ---
piece_t white_king = {
    .name       = "White King",
    .type       = PIECE_KING,
    .offsets    = {{+1, 0}, {+1, +1}, {0, +1}, {-1, +1}, {-1, 0}, {-1, -1}, {0, -1}, {+1, -1}},
    .offset_len = 8
};

piece_t white_queen = {
    .name       = "White Queen",
    .type       = PIECE_QUEEN,
    .offsets    = {{+1, 0}, {+1, +1}, {0, +1}, {-1, +1}, {-1, 0}, {-1, -1}, {0, -1}, {+1, -1}},
    .offset_len = 8
};

piece_t white_rook = {
    .name       = "White Rook",
    .type       = PIECE_ROOK,
    .offsets    = {{+1, 0}, {0, +1}, {-1, 0}, {0, -1}},
    .offset_len = 4
};

piece_t white_bishop = {
    .name       = "White Bishop",
    .type       = PIECE_BISHOP,
    .offsets    = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}},
    .offset_len = 4
};

piece_t white_knight = {
    .name       = "White Knight",
    .type       = PIECE_KNIGHT,
    .offsets    = {{+1, +2}, {+2, +1}, {+2, -1}, {+1, -2}, {-1, -2}, {-2, -1}, {-2, +1}, {-1, +2}},
    .offset_len = 8
};

piece_t white_pawn = {
    .name       = "White Pawn",
    .type       = PIECE_PAWN,
    .offsets    = {{0, 0}},
    .offset_len = 0};  // 폰은 특수 처리

// --- 블랙 기물들 ---
piece_t black_king = {
    .name       = "Black King",
    .type       = PIECE_KING,
    .offsets    = {{+1, 0}, {+1, +1}, {0, +1}, {-1, +1}, {-1, 0}, {-1, -1}, {0, -1}, {+1, -1}},
    .offset_len = 8
};

piece_t black_queen = {
    .name       = "Black Queen",
    .type       = PIECE_QUEEN,
    .offsets    = {{+1, 0}, {+1, +1}, {0, +1}, {-1, +1}, {-1, 0}, {-1, -1}, {0, -1}, {+1, -1}},
    .offset_len = 8
};

piece_t black_rook = {
    .name       = "Black Rook",
    .type       = PIECE_ROOK,
    .offsets    = {{+1, 0}, {0, +1}, {-1, 0}, {0, -1}},
    .offset_len = 4
};

piece_t black_bishop = {
    .name       = "Black Bishop",
    .type       = PIECE_BISHOP,
    .offsets    = {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}},
    .offset_len = 4
};

piece_t black_knight = {
    .name       = "Black Knight",
    .type       = PIECE_KNIGHT,
    .offsets    = {{+1, +2}, {+2, +1}, {+2, -1}, {+1, -2}, {-1, -2}, {-2, -1}, {-2, +1}, {-1, +2}},
    .offset_len = 8
};

piece_t black_pawn = {
    .name       = "Black Pawn",
    .type       = PIECE_PAWN,
    .offsets    = {{0, 0}},
    .offset_len = 0};

// --- 기물 테이블 & 승급용 퀸 반환 함수 ---
piece_t *piece_table[2][6] = {
    {&white_pawn, &white_knight, &white_bishop, &white_rook, &white_queen, &white_king},
    {&black_pawn, &black_knight, &black_bishop, &black_rook, &black_queen, &black_king}
};

piece_t *get_default_queen(color_t color) {
    return piece_table[color][PIECE_QUEEN];
}
