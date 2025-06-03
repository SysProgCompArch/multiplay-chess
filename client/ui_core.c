#define _XOPEN_SOURCE_EXTENDED
#include "ui.h"
#include "client_state.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>

// ncurses 초기화
void init_ncurses()
{
    setlocale(LC_ALL, "");

    initscr();

    // 터미널 크기 먼저 확인
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < 30 || cols < 107)
    {
        endwin();
        printf("터미널 크기가 너무 작습니다. 최소 %dx%d가 필요합니다.\n",
               107, 30);
        printf("현재 크기: %dx%d\n", cols, rows);
        exit(1);
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    curs_set(0);

    // 색상 초기화
    if (has_colors())
    {
        start_color();
        init_pair(COLOR_PAIR_BORDER, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_PAIR_WHITE_PIECE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_BLACK_PIECE, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_PAIR_BOARD_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair(COLOR_PAIR_BOARD_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_PAIR_SELECTED_SQUARE, COLOR_RED, COLOR_YELLOW);
    }

    clear();
    refresh();
}

// ncurses 정리
void cleanup_ncurses()
{
    if (isendwin() == FALSE)
    {
        endwin();
    }
}

// 테두리 그리기
void draw_border(WINDOW *win)
{
    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));
}

// 사용자명 입력 다이얼로그
bool get_username_dialog()
{
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

    if (strlen(input) > 0)
    {
        client_state_t *client = get_client_state();
        strncpy(client->username, input, sizeof(client->username) - 1);
        strncpy(client->game_state.local_player, input, sizeof(client->game_state.local_player) - 1);
        return true;
    }

    return false;
}