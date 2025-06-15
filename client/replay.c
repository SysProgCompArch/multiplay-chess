// client/replay.c

#include "replay.h"
#include "logger.h"         // LOG_ERROR, LOG_DEBUG
#include "ui/ui.h"          // draw_current_screen()
#include "game_state.h"     // apply_move_from_server(), reset_game_to_starting_position()
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MOVES 512
#define MAX_SAN    16

// PGN 파일에서 수(token)만 추출
static int load_moves(const char* filename, char moves[][MAX_SAN]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[512];
    // 헤더(태그) 건너뛰기
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n') break;
    }
    int count = 0;
    char token[32];
    while (fscanf(fp, "%31s", token) == 1 && count < MAX_MOVES) {
        // “1.” 같은 수번호 토큰 무시
        if (strchr(token, '.') != NULL) continue;
        strncpy(moves[count], token, MAX_SAN-1);
        moves[count][MAX_SAN-1] = '\0';
        count++;
    }
    fclose(fp);
    return count;
}

// 전역으로 init_game_state()에서 셋업되는 포인터
extern game_state_t *g_game_state;

void start_replay(const char* filename) {
    LOG_DEBUG("start_replay(): loading %s", filename);
    char moves[MAX_MOVES][MAX_SAN];
    int move_count = load_moves(filename, moves);
    if (move_count == 0) {
        LOG_ERROR("start_replay(): failed to load moves from %s", filename);
        return;
    }

    // 리플레이 전용으로 게임 상태 초기화
    reset_game_to_starting_position(&g_game_state->game);
    draw_current_screen();

    // 하단에 안내문구 표시
    mvprintw(LINES-1, 0, "Replay mode: n-next, p-prev, q-quit");
    refresh();

    int idx = 0, ch;
    while ((ch = getch()) != 'q') {
        if (ch == 'n' && idx < move_count) {
            // “e2e4” 같은 토큰을 from/to로 분리
            char from[3] = { moves[idx][0], moves[idx][1], '\0' };
            char to[3]   = { moves[idx][2], moves[idx][3], '\0' };
            apply_move_from_server(&g_game_state->game, from, to);
            idx++;
            draw_current_screen();
            mvprintw(LINES-1, 0, "Replay mode: n-next, p-prev, q-quit");
            refresh();
        }
        else if (ch == 'p' && idx > 0) {
            idx = (idx > 1 ? idx - 2 : 0);
            reset_game_to_starting_position(&g_game_state->game);
            for (int i = 0; i < idx; i++) {
                char from[3] = { moves[i][0], moves[i][1], '\0' };
                char to[3]   = { moves[i][2], moves[i][3], '\0' };
                apply_move_from_server(&g_game_state->game, from, to);
            }
            draw_current_screen();
            mvprintw(LINES-1, 0, "Replay mode: n-next, p-prev, q-quit");
            refresh();
        }
    }

    // 종료 시 안내문 지우기
    move(LINES-1, 0);
    clrtoeol();
    refresh();
}
