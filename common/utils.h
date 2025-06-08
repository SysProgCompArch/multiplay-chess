// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#include "rule.h"

// 보드 초기화 (모든 칸을 빈 칸으로)
void clear_board(game_t* G);

// 표준 시작위치 설정
void init_startpos(game_t* G);

// FEN 문자열을 game_t 로 파싱
// 성공 시 true, 포맷 오류 시 false
bool fen_parse(game_t* G, const char* fen);

#endif  // UTILS_H
