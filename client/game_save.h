// client/game_save.h
#ifndef GAME_SAVE_H
#define GAME_SAVE_H

#include "game_state.h"

/**
 * 게임 종료 시 game_state의 PGN 수들을
 * replays 폴더에 timestamped 파일로 저장
 */
void save_current_game(game_state_t* state);

#endif // GAME_SAVE_H