// client/replay.c

#include "replay.h"
#include "logger.h"         // LOG_ERROR, LOG_DEBUG
#include "ui/ui.h"          // draw_current_screen(), close_dialog(), enable_cursor_mode(), disable_cursor_mode()
#include "game_state.h"     // apply_move_from_server(), reset_game_to_starting_position()
#include "client_state.h"   // client_state_t 및 screen_mutex, add_chat_message_safe를 위해 추가
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // for strerror
#include <pthread.h> // pthread_mutex_lock, pthread_mutex_unlock을 위해 추가

#define MAX_MOVES 512
#define MAX_SAN    16

// PGN 파일에서 수(token)만 추출
static int load_moves(const char* filename, char moves[][MAX_SAN]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        LOG_ERROR("load_moves(): failed to open file %s: %s", filename, strerror(errno));
        return 0;
    }
    char line[512];
    // 헤더(태그) 건너뛰기
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n') break;
    }
    int count = 0;
    char token[32]; // 토큰을 더 크게 잡고
    while (fscanf(fp, "%31s", token) == 1 && count < MAX_MOVES) {
        // "1." 같은 수번호 토큰 무시
        if (strchr(token, '.') != NULL) continue;
        // PGN 결과 (1-0, 0-1, 1/2-1/2, *) 무시
        if (strcmp(token, "1-0") == 0 || strcmp(token, "0-1") == 0 ||
            strcmp(token, "1/2-1/2") == 0 || strcmp(token, "*") == 0) {
            continue;
        }
        strncpy(moves[count], token, MAX_SAN-1);
        moves[count][MAX_SAN-1] = '\0';
        LOG_DEBUG("Loaded move: %s", moves[count]);
        count++;
    }
    fclose(fp);
    LOG_INFO("load_moves(): Loaded %d moves from %s", count, filename);
    return count;
}


void start_replay(const char* filename) {
    LOG_INFO("start_replay(): entered, filename: %s", filename);

    client_state_t *client_state = get_client_state();
    if (!client_state) {
        LOG_ERROR("start_replay(): client_state is NULL");
        return;
    }

    // 리플레이 모드 진입 전 현재 화면 상태를 저장 (리플레이 종료 후 복원)
    screen_state_t prev_screen = client_state->current_screen;

    char moves[MAX_MOVES][MAX_SAN];
    int move_count = load_moves(filename, moves);
    if (move_count == 0) {
        LOG_ERROR("start_replay(): failed to load moves from %s", filename);
        // 사용자에게 메시지 표시
        show_dialog("Replay Error", "Failed to load replay file or no moves found.", "OK");
        return;
    }

    // 리플레이 모드 진입: 화면 상태를 게임 화면으로 변경
    pthread_mutex_lock(&screen_mutex); // UI 변경 전 뮤텍스 잠금
    client_state->current_screen = SCREEN_GAME; // 
    client_state->game_state.game_in_progress = false; // 리플레이 모드로 간주
    reset_game_to_starting_position(&client_state->game_state.game);
    client_state->game_state.white_in_check = false; // 체크 상태 초기화
    client_state->game_state.black_in_check = false;
    client_state->piece_selected = false; // 기물 선택 상태 초기화
    disable_cursor_mode(); // 커서 모드 비활성화
    pthread_mutex_unlock(&screen_mutex); // UI 변경 후 뮤텍스 해제

    // 화면 업데이트를 강제
    pthread_mutex_lock(&screen_mutex);
    draw_current_screen();
    pthread_mutex_unlock(&screen_mutex);
    refresh(); // UI 갱신을 강제

    // 하단에 안내문구 표시 (게임 화면으로 전환되었으므로 LINES-1은 여전히 유효)
    mvprintw(LINES-1, 0, "Replay mode: n-next, p-prev, q-quit, r-reset");
    clrtoeol(); // 기존 내용 지우기
    refresh();

    int idx = 0;
    int ch;
    while ((ch = getch()) != 'q') {
        pthread_mutex_lock(&screen_mutex); // UI/게임 상태 변경 전 뮤텍스 잠금

        if (ch == 'n' && idx < move_count) {
            // "e2e4" 같은 토큰을 from/to로 분리
            char from[3] = { moves[idx][0], moves[idx][1], '\0' };
            char to[3]   = { moves[idx][2], moves[idx][3], '\0' };
            LOG_DEBUG("Replay: Applying move %d: %s->%s", idx, from, to);
            if (apply_move_from_server(&client_state->game_state.game, from, to)) {
                LOG_DEBUG("Move %s->%s applied successfully.", from, to);
                // 리플레이 중 체크 상태 변화를 반영
                // apply_move_from_server는 게임 상태를 업데이트 하지만
                // is_check 여부 자체는 서버가 알려주지 않으므로 임시로 체크 해제
                // 실제 게임에서는 서버의 move_broadcast에 is_check 필드가 있음
                client_state->game_state.white_in_check = false;
                client_state->game_state.black_in_check = false;
                // 만약 체크메이트/스테일메이트 등 PGN 결과에 따라 현재 보드 상태를 판단하고 싶다면,
                // rule.c의 is_in_check, is_checkmate, is_stalemate 함수를 사용해 판단 후
                // client_state->game_state.white_in_check, black_in_check 등을 업데이트할 수 있음.
            } else {
                LOG_ERROR("Failed to apply move %s->%s (index %d). Skipping.", from, to, idx);
                add_chat_message_safe("Replay System", "Failed to apply move, PGN might be invalid.");
            }
            idx++;
        }
        else if (ch == 'p' && idx > 0) {
            idx = (idx > 1 ? idx - 2 : 0); // 이전 수의 시작으로 돌아가서 다시 그리기 위해 2칸 뒤로
            LOG_DEBUG("Replay: Moving to previous state. New index: %d", idx);
            reset_game_to_starting_position(&client_state->game_state.game); // 보드 초기화
            client_state->game_state.white_in_check = false; // 체크 상태 초기화
            client_state->game_state.black_in_check = false;
            for (int i = 0; i < idx; i++) {
                char from[3] = { moves[i][0], moves[i][1], '\0' };
                char to[3]   = { moves[i][2], moves[i][3], '\0' };
                if (!apply_move_from_server(&client_state->game_state.game, from, to)) {
                    LOG_ERROR("Replay: Failed to re-apply move %s->%s at index %d while moving back.", from, to, i);
                    add_chat_message_safe("Replay System", "Error replaying move, PGN might be invalid.");
                    break;
                }
            }
        }
        else if (ch == 'r') { // Reset to start
            LOG_INFO("Replay: Resetting to start position.");
            reset_game_to_starting_position(&client_state->game_state.game);
            client_state->game_state.white_in_check = false; // 체크 상태 초기화
            client_state->game_state.black_in_check = false;
            idx = 0;
        }
        else if (ch == ERR) { // Non-blocking getch() timeout
            // 일정 시간마다 화면을 다시 그리도록 허용 (예: 애니메이션 등)
        }
        else {
            LOG_DEBUG("Replay: Unhandled key: %d", ch);
        }

        // 게임 상태 변경 후 UI 업데이트 요청
        client_state->screen_update_requested = true;
        pthread_mutex_unlock(&screen_mutex); // 뮤텍스 해제

        // 화면 강제 업데이트
        pthread_mutex_lock(&screen_mutex);
        draw_current_screen();
        pthread_mutex_unlock(&screen_mutex);
        // 안내 문구 갱신
        mvprintw(LINES-1, 0, "Replay mode: n-next, p-prev, q-quit, r-reset. Move: %d/%d", idx, move_count);
        clrtoeol(); // 현재 줄의 나머지 부분을 지웁니다.
        refresh(); // UI 갱신을 강제
    }

    // 리플레이 종료 시 안내문 지우기
    move(LINES-1, 0);
    clrtoeol();
    refresh();
    
    LOG_INFO("start_replay(): exited.");

    // 리플레이 종료 후 이전 화면 상태로 복원
    pthread_mutex_lock(&screen_mutex);
    client_state->current_screen = prev_screen;
    // 게임 종료 후 필요한 상태 초기화 (예: 선택된 기물 해제)
    client_state->piece_selected = false;
    disable_cursor_mode(); // 커서 모드 비활성화
    // 마지막으로 화면을 다시 그려줍니다.
    draw_current_screen();
    pthread_mutex_unlock(&screen_mutex);
    refresh();
}