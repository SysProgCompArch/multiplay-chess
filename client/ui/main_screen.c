#include <string.h>

#include "../client_state.h"
#include "ui.h"

// 메인 화면 그리기
void draw_main_screen() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 화면 중앙 계산
    int main_height = 25;
    int main_width  = 80;
    int start_y     = (rows - main_height) / 2;
    int start_x     = (cols - main_width) / 2;

    // stdscr에 직접 그리기
    // ASCII 아트 제목
    attron(A_BOLD);
    mvprintw(start_y + 2, start_x + (main_width - 51) / 2, " ██████╗██╗  ██╗███████╗███████╗███████╗    ██████╗");
    mvprintw(start_y + 3, start_x + (main_width - 51) / 2, "██╔════╝██║  ██║██╔════╝██╔════╝██╔════╝   ██╔═══╝");
    mvprintw(start_y + 4, start_x + (main_width - 51) / 2, "██║     ███████║█████╗  ███████╗███████╗   ██║    ");
    mvprintw(start_y + 5, start_x + (main_width - 51) / 2, "██║     ██╔══██║██╔══╝  ╚════██║╚════██║   ██║    ");
    mvprintw(start_y + 6, start_x + (main_width - 51) / 2, "╚██████╗██║  ██║███████╗███████║███████║██╗╚██████╗");
    mvprintw(start_y + 7, start_x + (main_width - 51) / 2, " ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚═╝ ╚═════╝");
    attroff(A_BOLD);

    // 부제목
    attron(A_ITALIC);
    mvprintw(start_y + 9, start_x + (main_width - 16) / 2, "MULTIPLAYER GAME");
    attroff(A_ITALIC);

    // 장식선
    attron(COLOR_PAIR(COLOR_PAIR_BORDER));
    mvprintw(start_y + 11, start_x + (main_width - 60) / 2, "═══════════════════════════════════════════════════════════");
    attroff(COLOR_PAIR(COLOR_PAIR_BORDER));

    // 메뉴 항목들
    attron(A_BOLD);
    mvprintw(start_y + 14, start_x + (main_width - 18) / 2, "1. Start New Game");
    mvprintw(start_y + 16, start_x + (main_width - 18) / 2, "2. Replay        ");
    mvprintw(start_y + 18, start_x + (main_width - 18) / 2, "3. Exit          ");
    attroff(A_BOLD);

    // 하단 장식
    attron(COLOR_PAIR(COLOR_PAIR_BORDER));
    mvprintw(start_y + 21, start_x + (main_width - 60) / 2, "═══════════════════════════════════════════════════════════");
    attroff(COLOR_PAIR(COLOR_PAIR_BORDER));

    // 연결 상태 표시
    draw_connection_status();
}