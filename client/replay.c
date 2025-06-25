// client/replay.c

#include "replay.h"

#include <ncurses.h>
#include <signal.h>  // sig_atomic_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>  // usleep

#include "game_state.h"  // apply_move_from_server(), reset_game_to_starting_position()
#include "logger.h"      // LOG_ERROR, LOG_DEBUG
#include "ui/ui.h"       // draw_current_screen()

// 외부에서 정의된 종료 요청 플래그 (main.c에서)
extern volatile sig_atomic_t shutdown_requested;

#define MAX_MOVES 512
#define MAX_SAN   16

typedef struct {
    char           moves[MAX_MOVES][MAX_SAN];
    int            move_count;
    int            current_move;
    bool           auto_play;
    struct timeval last_auto_play_time;
} replay_state_t;

// PGN 파일에서 수(token)만 추출
static int load_moves(const char* filename, char moves[][MAX_SAN]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[512];
    // 헤더(태그) 건너뛰기
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n') break;
    }
    int  count = 0;
    char token[32];
    while (fscanf(fp, "%31s", token) == 1 && count < MAX_MOVES) {
        // "1." 같은 수번호 토큰 무시
        if (strchr(token, '.') != NULL) continue;
        strncpy(moves[count], token, MAX_SAN - 1);
        moves[count][MAX_SAN - 1] = '\0';
        count++;
    }
    fclose(fp);
    return count;
}

// 전역으로 init_game_state()에서 셋업되는 포인터
extern game_state_t* g_game_state;

// 리플레이 화면 그리기
static void draw_replay_screen(replay_state_t* state) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // clear() 제거 - 화면 깜박임 방지

    // 체스판 그리기
    WINDOW* board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, 2, 2);
    werase(board_win);  // 윈도우만 지우기
    draw_chess_board(board_win);
    wrefresh(board_win);

    // 행마 리스트 윈도우 (채팅창 위치에) - 위아래로 한 칸씩 확장
    int     moves_start_x = 2 + BOARD_WIDTH + 1;
    int     moves_height  = BOARD_HEIGHT;  // -2에서 0으로 변경 (2칸 확장)
    int     moves_width   = 35;
    WINDOW* moves_win     = newwin(moves_height, moves_width, 2, moves_start_x);  // 3에서 2로 변경 (1칸 위로)

    werase(moves_win);
    draw_border(moves_win);
    mvwprintw(moves_win, 0, 2, " Move List ");

    // 현재 화면에 표시할 수 있는 행마 수
    int display_height = moves_height - 2;
    int start_idx      = 0;

    // 현재 행마가 화면에 보이도록 스크롤 조정
    if (state->current_move >= display_height) {
        start_idx = state->current_move - display_height + 1;
    }

    // 행마 리스트 표시
    for (int i = 0; i < display_height && (start_idx + i) < state->move_count; i++) {
        int  move_idx = start_idx + i;
        int  move_num = move_idx / 2 + 1;
        bool is_white = (move_idx % 2 == 0);

        // 현재 행마 하이라이트
        if (move_idx == state->current_move) {
            wattron(moves_win, A_REVERSE);
        }

        if (is_white) {
            mvwprintw(moves_win, i + 1, 2, "%3d. %-8s", move_num, state->moves[move_idx]);
        } else {
            mvwprintw(moves_win, i + 1, 17, "%-8s", state->moves[move_idx]);
        }

        if (move_idx == state->current_move) {
            wattroff(moves_win, A_REVERSE);
        }
    }

    wrefresh(moves_win);

    // 하단 정보 영역 지우고 다시 그리기
    move(rows - 3, 0);
    clrtoeol();
    move(rows - 2, 0);
    clrtoeol();

    // 컨트롤 안내
    mvprintw(rows - 2, 2, "Controls: ↑: prev | ↓/Enter: next | Space: auto-play | Q/ESC: quit");
    mvprintw(rows - 3, 2, "Move %d/%d %s",
             state->current_move + 1, state->move_count,
             state->auto_play ? "[AUTO-PLAYING]" : "");

    refresh();
    delwin(board_win);
    delwin(moves_win);
}

// 현재 행마까지 게임 상태 재구성
static void replay_to_move(replay_state_t* state, int target_move) {
    reset_game_to_starting_position(&g_game_state->game);

    for (int i = 0; i <= target_move && i < state->move_count; i++) {
        if (strlen(state->moves[i]) >= 4) {
            char from[3] = {state->moves[i][0], state->moves[i][1], '\0'};
            char to[3]   = {state->moves[i][2], state->moves[i][3], '\0'};
            apply_move_from_server(&g_game_state->game, from, to);
        }
    }
}

// 시간 차이를 밀리초로 반환
static long time_diff_ms(struct timeval* t1, struct timeval* t2) {
    return (t2->tv_sec - t1->tv_sec) * 1000 + (t2->tv_usec - t1->tv_usec) / 1000;
}

void start_replay(const char* filename) {
    LOG_DEBUG("start_replay(): loading %s", filename);

    replay_state_t state;
    memset(&state, 0, sizeof(state));

    state.move_count = load_moves(filename, state.moves);
    if (state.move_count == 0) {
        LOG_ERROR("start_replay(): failed to load moves from %s", filename);
        return;
    }

    state.current_move = -1;  // 시작 위치 (움직임 없음)
    state.auto_play    = false;

    // 리플레이 전용으로 게임 상태 초기화
    reset_game_to_starting_position(&g_game_state->game);

    // 화면 완전히 지우기 (메인화면 잔상 제거)
    clear();
    refresh();

    // 키 입력 모드 설정 (메인 루프와 동일한 방식)
    keypad(stdscr, TRUE);

    // 초기 화면 그리기
    bool need_redraw = true;

    int ch;
    while (1) {
        bool state_changed = false;

        // 자동 재생 처리
        if (state.auto_play && state.current_move < state.move_count - 1) {
            struct timeval now;
            gettimeofday(&now, NULL);

            if (time_diff_ms(&state.last_auto_play_time, &now) >= 500) {
                state.current_move++;
                replay_to_move(&state, state.current_move);
                gettimeofday(&state.last_auto_play_time, NULL);
                state_changed = true;
            }
        }

        // 키 입력 처리 (메인 루프와 동일한 타임아웃 방식)
        // 자동 재생 중일 때는 더 짧은 타임아웃, 아니면 일반 타임아웃
        timeout(state.auto_play ? 50 : 100);  // 자동재생: 50ms, 수동: 100ms
        ch = getch();

        // getch() 이후 시그널 상태 확인 (메인 루프와 동일한 방식)
        if (shutdown_requested) {
            LOG_INFO("Shutdown requested in replay, exiting");
            break;
        }

        // getch()가 시그널에 의해 중단된 경우도 시그널 상태 확인
        if (ch == ERR && shutdown_requested) {
            LOG_INFO("getch() interrupted by signal in replay, shutting down");
            break;
        }

        if (ch != ERR) {                               // 키 입력이 있는 경우에만 처리
            if (ch == 'q' || ch == 'Q' || ch == 27) {  // ESC도 종료 키로 추가
                break;
            } else if (ch == KEY_UP && state.current_move >= 0) {
                // 이전 행마
                state.current_move--;
                replay_to_move(&state, state.current_move);
                state.auto_play = false;  // 수동 조작 시 자동 재생 중지
                state_changed   = true;
            } else if ((ch == KEY_DOWN || ch == '\n') && state.current_move < state.move_count - 1) {
                // 다음 행마 (Enter 키도 다음 행마로 변경)
                state.current_move++;
                replay_to_move(&state, state.current_move);
                state.auto_play = false;  // 수동 조작 시 자동 재생 중지
                state_changed   = true;
            } else if (ch == ' ') {
                // 자동 재생 토글
                state.auto_play = !state.auto_play;
                if (state.auto_play) {
                    gettimeofday(&state.last_auto_play_time, NULL);
                }
                state_changed = true;
            }
        }

        // 상태가 변경되었거나 처음 그리는 경우에만 화면 업데이트
        if (need_redraw || state_changed) {
            draw_replay_screen(&state);
            need_redraw = false;
        }

        // timeout() 방식을 사용하므로 별도의 sleep 불필요
    }

    // 키 입력 모드 복원 (timeout을 기본값으로 되돌림)
    timeout(-1);  // blocking mode로 복원

    // 화면 정리 및 메인 화면 복원
    clear();
    refresh();

    // Ctrl+C로 종료된 경우 메인 프로그램도 종료되도록 함
    if (shutdown_requested) {
        LOG_INFO("Replay exiting due to shutdown request, will propagate to main");
        // 메인 루프에서 shutdown_requested를 체크하여 정상 종료 처리됨
        return;
    }
}
