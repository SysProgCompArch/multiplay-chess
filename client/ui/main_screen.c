#include <string.h>

#include "../client_state.h"
#include "ui.h"

// 메인 화면 그리기
void draw_main_screen() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 더 큰 메인 윈도우 크기
    int main_height = 25;
    int main_width  = 80;
    int start_y     = (rows - main_height) / 2;
    int start_x     = (cols - main_width) / 2;

    WINDOW *main_win = newwin(main_height, main_width, start_y, start_x);
    werase(main_win);  // 윈도우만 지우기

    // ASCII 아트 제목
    wattron(main_win, A_BOLD);
    mvwprintw(main_win, 2, (main_width - 51) / 2, " ██████╗██╗  ██╗███████╗███████╗███████╗    ██████╗");
    mvwprintw(main_win, 3, (main_width - 51) / 2, "██╔════╝██║  ██║██╔════╝██╔════╝██╔════╝   ██╔═══╝");
    mvwprintw(main_win, 4, (main_width - 51) / 2, "██║     ███████║█████╗  ███████╗███████╗   ██║    ");
    mvwprintw(main_win, 5, (main_width - 51) / 2, "██║     ██╔══██║██╔══╝  ╚════██║╚════██║   ██║    ");
    mvwprintw(main_win, 6, (main_width - 51) / 2, "╚██████╗██║  ██║███████╗███████║███████║██╗╚██████╗");
    mvwprintw(main_win, 7, (main_width - 51) / 2, " ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝ ╚═════╝");
    wattroff(main_win, A_BOLD);

    // 부제목
    wattron(main_win, A_ITALIC);
    mvwprintw(main_win, 9, (main_width - 16) / 2, "MULTIPLAYER GAME");
    wattroff(main_win, A_ITALIC);

    // 장식선
    wattron(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));
    mvwprintw(main_win, 11, (main_width - 60) / 2, "═══════════════════════════════════════════════════════════");
    wattroff(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));

    // 메뉴 항목들
    wattron(main_win, A_BOLD);
    mvwprintw(main_win, 14, (main_width - 18) / 2, "1. Start New Game");
    mvwprintw(main_win, 16, (main_width - 18) / 2, "2. Replay        ");
    mvwprintw(main_win, 18, (main_width - 18) / 2, "3. Exit          ");
    wattroff(main_win, A_BOLD);

    // 하단 장식
    wattron(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));
    mvwprintw(main_win, 21, (main_width - 60) / 2, "═══════════════════════════════════════════════════════════");
    wattroff(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));

    // 윈도우 내용을 stdscr에 복사한 후 윈도우 삭제
    // 이렇게 하면 delwin() 후에도 내용이 stdscr에 남아있음
    copywin(main_win, stdscr, 0, 0, start_y, start_x, start_y + main_height - 1, start_x + main_width - 1, 0);
    delwin(main_win);  // 윈도우 메모리 해제

    // 연결 상태 표시
    draw_connection_status();
}