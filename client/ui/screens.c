#include <string.h>

#include "../client_state.h"
#include "ui.h"

// 재연결 상태 표시
void draw_connection_status() {
    client_state_t *client = get_client_state();
    int             rows, cols;
    getmaxyx(stdscr, rows, cols);

    pthread_mutex_lock(&network_mutex);
    if (client->reconnecting) {
        // 화면 우상단에 "Reconnecting..." 표시
        attron(COLOR_PAIR(COLOR_PAIR_BORDER));
        mvprintw(0, cols - 20, "Reconnecting...     ");
        attroff(COLOR_PAIR(COLOR_PAIR_BORDER));
    } else if (!client->connected) {
        // 화면 우상단에 "Disconnected" 표시
        attron(COLOR_PAIR(COLOR_PAIR_BORDER));
        mvprintw(0, cols - 20, "Disconnected        ");
        attroff(COLOR_PAIR(COLOR_PAIR_BORDER));
    } else {
        // 연결된 경우 상태 표시 지우기
        mvprintw(0, cols - 20, "                    ");
    }
    pthread_mutex_unlock(&network_mutex);
}