#ifndef PGN_SAVE_H
#define PGN_SAVE_H

#include "game_state.h"

/**
 * state->pgn_moves[], pgn_move_count, pgn_result 로부터
 * 완전한 PGN 파일을 생성합니다.
 */
int write_pgn(const char* filename, game_state_t* state);

#endif  // PGN_SAVE_H