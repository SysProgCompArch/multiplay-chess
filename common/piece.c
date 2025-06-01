// piece.c
#include "piece.h"

// --- 오프셋 리스트 매크로 정의 ---
#define KING_OFFSETS   { {+1,0},{+1,+1},{0,+1},{-1,+1}, \
                         {-1,0},{-1,-1},{0,-1},{+1,-1} }
#define QUEEN_OFFSETS  KING_OFFSETS
#define ROOK_OFFSETS   { {+1,0},{0,+1},{-1,0},{0,-1} }
#define BISHOP_OFFSETS { {+1,+1},{-1,+1},{-1,-1},{+1,-1} }
#define KNIGHT_OFFSETS { {+1,+2},{+2,+1},{+2,-1},{+1,-2}, \
                         {-1,-2},{-2,-1},{-2,+1},{-1,+2} }

// --- 화이트 기물들 ---
piece_t white_king   = { "White King",   KING,   KING_OFFSETS,    8 };
piece_t white_queen  = { "White Queen",  QUEEN,  QUEEN_OFFSETS,   8 };
piece_t white_rook   = { "White Rook",   ROOK,   ROOK_OFFSETS,    4 };
piece_t white_bishop = { "White Bishop", BISHOP, BISHOP_OFFSETS,  4 };
piece_t white_knight = { "White Knight", KNIGHT, KNIGHT_OFFSETS,  8 };
piece_t white_pawn   = { "White Pawn",   PAWN,   {{0,0}},         0 };  // 폰은 특수 처리

// --- 블랙 기물들 ---
piece_t black_king   = { "Black King",   KING,   KING_OFFSETS,    8 };
piece_t black_queen  = { "Black Queen",  QUEEN,  QUEEN_OFFSETS,   8 };
piece_t black_rook   = { "Black Rook",   ROOK,   ROOK_OFFSETS,    4 };
piece_t black_bishop = { "Black Bishop", BISHOP, BISHOP_OFFSETS,  4 };
piece_t black_knight = { "Black Knight", KNIGHT, KNIGHT_OFFSETS,  8 };
piece_t black_pawn   = { "Black Pawn",   PAWN,   {{0,0}},         0 };

// --- 기물 테이블 & 승격용 퀸 반환 함수 ---
piece_t* piece_table[2][6] = {
    { &white_king,   &white_queen,   &white_rook,
      &white_bishop, &white_knight,  &white_pawn },
    { &black_king,   &black_queen,   &black_rook,
      &black_bishop, &black_knight,  &black_pawn }
};

piece_t* get_default_queen(int color) {
    return piece_table[color][QUEEN];
}
