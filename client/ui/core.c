#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "../client_state.h"
#include "ui.h"

// ncurses 초기화
void init_ncurses() {
    setlocale(LC_ALL, "");

    initscr();

    // 터미널 크기 먼저 확인
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < 30 || cols < 100) {  // 2+60+1+35+2 = 100 (최소 구성)
        endwin();
        printf("터미널 크기가 너무 작습니다. 최소 %dx%d가 필요합니다.\n",
               100, 30);
        printf("현재 크기: %dx%d\n", cols, rows);
        exit(1);
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    curs_set(0);

    init_colors();

    clear();
    refresh();
}

// ncurses 정리
void cleanup_ncurses() {
    if (isendwin() == FALSE) {
        endwin();
    }
}

// 터미널 크기 변경 처리
void handle_terminal_resize() {
    // ncurses 화면 크기 업데이트
    endwin();
    refresh();

    // 터미널 크기 확인
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 최소 크기 확인
    if (rows < 30 || cols < 100) {  // 2+60+1+35+2 = 100 (최소 구성)
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "Terminal size is too small.\nMinimum required: 100x30\nCurrent size: %dx%d",
                 cols, rows);
        show_error_dialog("Terminal Too Small", error_msg);
        return;
    }

    // 현재 화면을 다시 그리기
    client_state_t *client = get_client_state();

    pthread_mutex_lock(&screen_mutex);
    draw_current_screen();
    pthread_mutex_unlock(&screen_mutex);

    refresh();
}

// 테두리 그리기
void draw_border(WINDOW *win) {
    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));
}

// 화면 전환 공통 함수
void draw_current_screen() {
    client_state_t *client = get_client_state();

    // mutex는 호출하는 쪽에서 처리하도록 변경
    screen_state_t current_screen = client->current_screen;

    switch (current_screen) {
        case SCREEN_MAIN:
            draw_main_screen();
            break;
        case SCREEN_MATCHING:
            draw_matching_screen();
            break;
        case SCREEN_GAME:
            draw_game_screen();
            break;
        default:
            draw_main_screen();
            break;
    }
}

// 사용자명 입력 다이얼로그
bool get_username_dialog() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - DIALOG_HEIGHT) / 2;
    int start_x = (cols - DIALOG_WIDTH) / 2;

    WINDOW *dialog = newwin(DIALOG_HEIGHT, DIALOG_WIDTH, start_y, start_x);
    draw_border(dialog);

    mvwprintw(dialog, 2, (DIALOG_WIDTH - 20) / 2, "Enter your username:");
    mvwprintw(dialog, 4, 2, "Name: ");

    wrefresh(dialog);

    echo();
    curs_set(1);

    char input[32] = {0};
    mvwgetnstr(dialog, 4, 8, input, 30);

    noecho();
    curs_set(0);

    delwin(dialog);

    if (strlen(input) > 0) {
        client_state_t *client = get_client_state();
        strncpy(client->username, input, sizeof(client->username) - 1);
        return true;
    }

    return false;
}

// 에러 다이얼로그 표시
void show_error_dialog(const char *title, const char *message) {
    client_state_t *client = get_client_state();

    pthread_mutex_lock(&screen_mutex);
    client->error_dialog_active = true;
    pthread_mutex_unlock(&screen_mutex);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int dialog_width  = 60;
    int dialog_height = 10;
    int start_y       = (rows - dialog_height) / 2;
    int start_x       = (cols - dialog_width) / 2;

    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);

    // 배경을 어둡게 만들기 위해 색상 설정
    wbkgd(dialog, COLOR_PAIR(COLOR_PAIR_DIALOG_BORDER));
    draw_border(dialog);

    // 제목 표시
    mvwprintw(dialog, 1, (dialog_width - strlen(title)) / 2, "%s", title);

    // 구분선
    mvwprintw(dialog, 2, 2, "========================================================");

    // 메시지 표시 (여러 줄 지원)
    char *msg_copy = strdup(message);
    char *line     = strtok(msg_copy, "\n");
    int   line_num = 4;

    while (line && line_num < dialog_height - 2) {
        int msg_len = strlen(line);
        if (msg_len > dialog_width - 4) {
            // 긴 메시지는 잘라서 표시
            char truncated[dialog_width - 3];
            strncpy(truncated, line, dialog_width - 7);
            truncated[dialog_width - 7] = '\0';
            strcat(truncated, "...");
            mvwprintw(dialog, line_num, 2, "%s", truncated);
        } else {
            mvwprintw(dialog, line_num, 2, "%s", line);
        }
        line = strtok(NULL, "\n");
        line_num++;
    }

    free(msg_copy);

    // 확인 버튼 안내
    mvwprintw(dialog, dialog_height - 2, (dialog_width - 20) / 2, "Press any key to close");

    wrefresh(dialog);

    // 키 입력 대기
    timeout(-1);  // 블로킹 모드로 변경
    getch();
    timeout(1000);  // 다시 타임아웃 모드로 복원

    delwin(dialog);

    pthread_mutex_lock(&screen_mutex);
    client->error_dialog_active = false;
    pthread_mutex_unlock(&screen_mutex);

    // 에러 다이얼로그를 닫은 후 즉시 현재 화면을 다시 그리기
    pthread_mutex_lock(&screen_mutex);
    draw_current_screen();
    pthread_mutex_unlock(&screen_mutex);

    refresh();
}