#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>  // mkdir
#include <time.h>

#include "game_state.h"
#include "logger.h"  // LOG_INFO, LOG_ERROR
#include "pgn_save.h"

/**
 * 게임이 종료되었을 때 state->pgn_moves 와 state->pgn_result 를
 * timestamped PGN 파일로 replays/ 폴더에 저장합니다.
 */
void save_current_game(game_state_t* state) {
    LOG_INFO("save_current_game called");

    // 이동 기록이 없으면 저장하지 않음
    if (state->pgn_move_count == 0) {
        LOG_INFO("No moves recorded, skipping PGN save");
        return;
    }

    LOG_INFO("Saving game with %d moves", state->pgn_move_count);

    // replays 폴더가 없으면 생성
    if (mkdir("replays", 0755) && errno != EEXIST) {
        LOG_ERROR("Failed to create replays directory: %s", strerror(errno));
        return;
    } else {
        LOG_INFO("replays directory ready");
    }

    // 1) 체크메이트: 현 수 차례(side_to_move)가 패배
    if (is_checkmate(&state->game)) {
        // 현 수 차례가 로컬이라면 로컬 패배, 아니면 로컬 승리
        if (state->game.side_to_move == state->local_team) {
            // local lost
            strcpy(state->pgn_result,
                   state->local_team == TEAM_WHITE ? "0-1" : "1-0");
        } else {
            // local won
            strcpy(state->pgn_result,
                   state->local_team == TEAM_WHITE ? "1-0" : "0-1");
        }
    }
    // 2) 스테일메이트: 무승부
    else if (is_stalemate(&state->game)) {
        strcpy(state->pgn_result, "1/2-1/2");
    }
    // 3) 상대 연결 끊김 또는 항복: 로컬 승리
    else if (state->opponent_disconnected) {
        strcpy(state->pgn_result,
               state->local_team == TEAM_WHITE ? "1-0" : "0-1");
    }
    // 4) 기타 종료(타임아웃 등): 결과 미표시
    else {
        strcpy(state->pgn_result, "*");
    }

    // timestamped filename 생성
    char      filename[256];
    time_t    t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    strftime(filename, sizeof(filename),
             "replays/game_%Y%m%d_%H%M%S.pgn", &tm_info);

    LOG_INFO("Attempting to save game with %d moves to %s", state->pgn_move_count, filename);
    LOG_INFO("Game result: %s", state->pgn_result);

    // PGN 쓰기
    if (write_pgn(filename, state) == 0) {
        LOG_INFO("Game saved successfully to %s", filename);
    } else {
        LOG_ERROR("Failed to save game to PGN file %s", filename);
    }
}
