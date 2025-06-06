#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "../client_state.h"
#include "ui.h"

// 애니메이션을 위한 전역 변수
static int            animation_frame     = 0;
static struct timeval last_animation_time = {0, 0};

// 애니메이션 프레임 업데이트 함수
void update_animation_frame() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    // 경과 시간 계산 (밀리초 단위)
    long elapsed_ms = (current_time.tv_sec - last_animation_time.tv_sec) * 1000 +
                      (current_time.tv_usec - last_animation_time.tv_usec) / 1000;

    if (elapsed_ms >= 300) {  // 0.3초(300ms)마다 프레임 업데이트
        animation_frame     = (animation_frame + 1) % 4;
        last_animation_time = current_time;
    }
}

// 매칭 화면 그리기
void draw_matching_screen() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MATCH_DIALOG_HEIGHT) / 2;
    int start_x = (cols - MATCH_DIALOG_WIDTH) / 2;

    WINDOW *match_win = newwin(MATCH_DIALOG_HEIGHT, MATCH_DIALOG_WIDTH, start_y, start_x);
    werase(match_win);  // 윈도우만 지우기
    draw_border(match_win);

    // 애니메이션 프레임 업데이트
    update_animation_frame();

    // 애니메이션 효과가 있는 메시지 생성
    char        message[32];
    const char *dots[] = {"", ".", "..", "..."};
    snprintf(message, sizeof(message), "Finding opponent%s", dots[animation_frame]);

    wattron(match_win, A_BOLD);
    mvwprintw(match_win, 2, (MATCH_DIALOG_WIDTH - strlen(message)) / 2, "%s", message);
    wattroff(match_win, A_BOLD);

    client_state_t *client       = get_client_state();
    time_t          current_time = time(NULL);
    int             elapsed      = (int)(current_time - client->match_start_time);
    int             minutes      = elapsed / 60;
    int             seconds      = elapsed % 60;

    mvwprintw(match_win, 4, (MATCH_DIALOG_WIDTH - 16) / 2, "Wait time: %02d:%02d", minutes, seconds);

    // 연결 상태 표시
    pthread_mutex_lock(&network_mutex);
    const char *status = client->connected ? "Connected" : "Connecting...";
    pthread_mutex_unlock(&network_mutex);

    mvwprintw(match_win, 7, (MATCH_DIALOG_WIDTH - 19) / 2, "Press ESC to cancel");

    wrefresh(match_win);
    delwin(match_win);  // 윈도우 메모리 해제

    // 연결 상태 표시
    draw_connection_status();
}

// 매칭 타이머 업데이트 (매초 호출)
void update_match_timer() {
    client_state_t *client = get_client_state();

    if (client->current_screen == SCREEN_MATCHING) {
        // 타임아웃 체크 (30초)
        time_t current_time = time(NULL);
        if (current_time - client->match_start_time >= 30) {
            add_chat_message_safe("System", "Matchmaking timeout");
            client->current_screen = SCREEN_MAIN;
        }
    }
}