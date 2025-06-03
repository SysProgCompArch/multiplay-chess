// piece.c
#include "piece.h"

// --- 오프셋 리스트 매크로 정의 ---
#define KING_OFFSETS {{+1, 0}, {+1, +1}, {0, +1}, {-1, +1}, {-1, 0}, {-1, -1}, {0, -1}, {+1, -1}}
#define QUEEN_OFFSETS KING_OFFSETS
#define ROOK_OFFSETS {{+1, 0}, {0, +1}, {-1, 0}, {0, -1}}
#define BISHOP_OFFSETS {{+1, +1}, {-1, +1}, {-1, -1}, {+1, -1}}
#define KNIGHT_OFFSETS {{+1, +2}, {+2, +1}, {+2, -1}, {+1, -2}, {-1, -2}, {-2, -1}, {-2, +1}, {-1, +2}}

// --- 화이트 기물들 ---
piece_t white_king = {.name = "White King", .type = KING, .offsets = KING_OFFSETS, .offset_len = 8};
piece_t white_queen = {.name = "White Queen", .type = QUEEN, .offsets = QUEEN_OFFSETS, .offset_len = 8};
piece_t white_rook = {.name = "White Rook", .type = ROOK, .offsets = ROOK_OFFSETS, .offset_len = 4};
piece_t white_bishop = {.name = "White Bishop", .type = BISHOP, .offsets = BISHOP_OFFSETS, .offset_len = 4};
piece_t white_knight = {.name = "White Knight", .type = KNIGHT, .offsets = KNIGHT_OFFSETS, .offset_len = 8};
piece_t white_pawn = {.name = "White Pawn", .type = PAWN, .offsets = {{0, 0}}, .offset_len = 0}; // 폰은 특수 처리

// --- 블랙 기물들 ---
piece_t black_king = {.name = "Black King", .type = KING, .offsets = KING_OFFSETS, .offset_len = 8};
piece_t black_queen = {.name = "Black Queen", .type = QUEEN, .offsets = QUEEN_OFFSETS, .offset_len = 8};
piece_t black_rook = {.name = "Black Rook", .type = ROOK, .offsets = ROOK_OFFSETS, .offset_len = 4};
piece_t black_bishop = {.name = "Black Bishop", .type = BISHOP, .offsets = BISHOP_OFFSETS, .offset_len = 4};
piece_t black_knight = {.name = "Black Knight", .type = KNIGHT, .offsets = KNIGHT_OFFSETS, .offset_len = 8};
piece_t black_pawn = {.name = "Black Pawn", .type = PAWN, .offsets = {{0, 0}}, .offset_len = 0};

// --- 기물 테이블 & 승격용 퀸 반환 함수 ---
piece_t *piece_table[2][6] = {
    {&white_king, &white_queen, &white_rook,
     &white_bishop, &white_knight, &white_pawn},
    {&black_king, &black_queen, &black_rook,
     &black_bishop, &black_knight, &black_pawn}};

piece_t *get_default_queen(int color)
{
  return piece_table[color][QUEEN];
}
