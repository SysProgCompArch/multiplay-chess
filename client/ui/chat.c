#include <ncursesw/ncurses.h>
#include <string.h>

#include "../client_network.h"
#include "../client_state.h"
#include "logger.h"
#include "ui.h"

// 채팅 입력 모드 활성화
void enable_chat_input() {
    client_state_t *client = get_client_state();
    LOG_DEBUG("Enabling chat input mode");

    client->chat_input_mode = true;
    memset(client->chat_input_buffer, 0, sizeof(client->chat_input_buffer));
    client->chat_input_cursor = 0;
}

// 채팅 입력 모드 비활성화
void disable_chat_input() {
    client_state_t *client = get_client_state();
    LOG_DEBUG("Disabling chat input mode");

    client->chat_input_mode = false;
    memset(client->chat_input_buffer, 0, sizeof(client->chat_input_buffer));
    client->chat_input_cursor = 0;
}

// 채팅 입력 처리
void handle_chat_input(int ch) {
    client_state_t *client = get_client_state();

    if (!client->chat_input_mode) {
        return;
    }

    switch (ch) {
        case '\n':
        case '\r':
            // 엔터키 - 메시지 전송
            if (client->chat_input_cursor > 0) {
                client->chat_input_buffer[client->chat_input_cursor] = '\0';
                LOG_INFO("Sending chat message: %s", client->chat_input_buffer);
                send_chat_message(client->chat_input_buffer);
            }
            disable_chat_input();
            break;

        case 27:  // ESC
            // ESC키 - 입력 취소
            LOG_DEBUG("Chat input cancelled");
            disable_chat_input();
            break;

        case KEY_BACKSPACE:
        case 8:
        case 127:
            // 백스페이스
            if (client->chat_input_cursor > 0) {
                client->chat_input_cursor--;
                client->chat_input_buffer[client->chat_input_cursor] = '\0';
            }
            break;

        default:
            // 일반 문자 입력
            if (ch >= 32 && ch <= 126 && client->chat_input_cursor < sizeof(client->chat_input_buffer) - 1) {
                client->chat_input_buffer[client->chat_input_cursor] = ch;
                client->chat_input_cursor++;
            }
            break;
    }
}

// 채팅 입력창 그리기
void draw_chat_input(WINDOW *input_win) {
    client_state_t *client = get_client_state();

    werase(input_win);
    draw_border(input_win);

    // 입력창 크기 확인
    int win_height, win_width;
    getmaxyx(input_win, win_height, win_width);

    if (client->chat_input_mode) {
        // 입력 모드일 때
        int max_display_len = win_width - 12;  // "Message: " + 테두리 여백

        // 입력 버퍼가 너무 길면 끝부분만 표시
        char display_buffer[256];
        int  buffer_len = strlen(client->chat_input_buffer);

        if (buffer_len > max_display_len) {
            strncpy(display_buffer, client->chat_input_buffer + (buffer_len - max_display_len), max_display_len);
            display_buffer[max_display_len] = '\0';
        } else {
            strcpy(display_buffer, client->chat_input_buffer);
        }

        mvwprintw(input_win, 1, 2, "Message: %s", display_buffer);

        // 커서 표시
        int display_cursor = strlen(display_buffer);
        int cursor_x       = 11 + display_cursor;  // "Message: " 길이
        if (cursor_x < win_width - 3) {            // 윈도우 경계 체크
            mvwprintw(input_win, 1, cursor_x, "_");
        }

        // 하단에 도움말 표시
        mvwprintw(input_win, 2, 2, "Enter: Send, ESC: Cancel");
    } else {
        // 비활성 모드일 때
        mvwprintw(input_win, 1, 2, "Press Enter to chat");
    }

    wrefresh(input_win);
}