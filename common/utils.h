// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include "rule.h"

// 표준 시작위치 설정
void init_startpos(game_t* G);

// FEN 문자열을 game_t 로 파싱
// 성공 시 true, 포맷 오류 시 false
bool fen_parse(const char* fen, game_t* G);

#endif // UTILS_H
