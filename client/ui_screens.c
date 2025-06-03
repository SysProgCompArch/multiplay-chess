#include "ui.h"
#include "client_state.h"
#include <time.h>
#include <string.h>

// 메인 화면 그리기
void draw_main_screen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MAIN_MENU_HEIGHT) / 2;
    int start_x = (cols - MAIN_MENU_WIDTH) / 2;

    WINDOW *main_win = newwin(MAIN_MENU_HEIGHT, MAIN_MENU_WIDTH, start_y, start_x);
    draw_border(main_win);

    mvwprintw(main_win, 2, (MAIN_MENU_WIDTH - 18) / 2, "MULTIPLAYER CHESS");
    mvwprintw(main_win, 4, (MAIN_MENU_WIDTH - 30) / 2, "==============================");
    mvwprintw(main_win, 7, (MAIN_MENU_WIDTH - 20) / 2, "1. Start Game");
    mvwprintw(main_win, 8, (MAIN_MENU_WIDTH - 20) / 2, "2. Options");
    mvwprintw(main_win, 9, (MAIN_MENU_WIDTH - 20) / 2, "3. Exit");
    mvwprintw(main_win, 12, (MAIN_MENU_WIDTH - 35) / 2, "Press 1, 2, 3 or use mouse");

    wrefresh(main_win);
}

// 매칭 화면 그리기
void draw_matching_screen()
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MATCH_DIALOG_HEIGHT) / 2;
    int start_x = (cols - MATCH_DIALOG_WIDTH) / 2;

    WINDOW *match_win = newwin(MATCH_DIALOG_HEIGHT, MATCH_DIALOG_WIDTH, start_y, start_x);
    draw_border(match_win);

    mvwprintw(match_win, 2, (MATCH_DIALOG_WIDTH - 20) / 2, "Finding opponent...");

    client_state_t *client = get_client_state();
    time_t current_time = time(NULL);
    int elapsed = (int)(current_time - client->match_start_time);
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;

    mvwprintw(match_win, 4, (MATCH_DIALOG_WIDTH - 20) / 2, "Wait time: %02d:%02d", minutes, seconds);

    // 연결 상태 표시
    pthread_mutex_lock(&network_mutex);
    const char *status = client->connected ? "Connected" : "Connecting...";
    pthread_mutex_unlock(&network_mutex);

    mvwprintw(match_win, 5, (MATCH_DIALOG_WIDTH - 25) / 2, "Server: %s", status);
    mvwprintw(match_win, 6, (MATCH_DIALOG_WIDTH - 25) / 2, "Searching for player...");
    mvwprintw(match_win, 8, (MATCH_DIALOG_WIDTH - 30) / 2, "Press ESC to cancel");

    wrefresh(match_win);
}

// 체스 말 ASCII 문자 반환
char get_piece_char(piece_t *piece, int team)
{
    if (!piece)
        return ' ';

    // 흰색 팀 (0) - 대문자, 검은색 팀 (1) - 소문자
    if (team == 0)
    { // 흰색
        switch (piece->type)
        {
        case KING:
            return 'K';
        case QUEEN:
            return 'Q';
        case ROOK:
            return 'R';
        case BISHOP:
            return 'B';
        case KNIGHT:
            return 'N';
        case PAWN:
            return 'P';
        }
    }
    else
    { // 검은색
        switch (piece->type)
        {
        case KING:
            return 'k';
        case QUEEN:
            return 'q';
        case ROOK:
            return 'r';
        case BISHOP:
            return 'b';
        case KNIGHT:
            return 'n';
        case PAWN:
            return 'p';
        }
    }
    return ' ';
}

// 체스 보드 그리기
void draw_chess_board(WINDOW *board_win)
{
    client_state_t *client = get_client_state();

    werase(board_win);
    draw_border(board_win);

    // 상단 라벨 (테두리+라벨)
    mvwprintw(board_win, 1, BOARD_LABEL_Y, "     a      b      c      d      e      f      g      h");

    board_state_t *board = &client->game_state.board_state;

    for (int row = 0; row < BOARD_SIZE; row++)
    {
        // 왼쪽 라벨: 테두리 다음 칸(1)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, 1, "%d ", 8 - row);
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            int y = 2 + row * SQUARE_H;
            int x = BOARD_LABEL_X + col * SQUARE_W;
            bool is_selected = (client->piece_selected &&
                                client->selected_x == col &&
                                client->selected_y == row);
            // 색상
            if (is_selected)
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            }
            else if ((row + col) % 2 == 0)
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            else
            {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
            // 3줄로 한 칸 출력
            mvwprintw(board_win, y, x, "       ");
            piecestate_t *piece = &board->board[row][col];
            if (piece->piece && !piece->is_dead)
            {
                char piece_char = get_piece_char(piece->piece, 0); // 임시로 흰색
                mvwprintw(board_win, y + 1, x, "   %c   ", piece_char);
            }
            else
            {
                mvwprintw(board_win, y + 1, x, "       ");
            }
            mvwprintw(board_win, y + 2, x, "       ");
            // 색상 해제
            if (is_selected)
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            }
            else if ((row + col) % 2 == 0)
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            else
            {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
        }
        // 오른쪽 라벨: 테두리 바로 안쪽(BOARD_WIDTH - 2)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, BOARD_WIDTH - 2, "%d", 8 - row);
    }
    // 하단 라벨
    mvwprintw(board_win, BOARD_HEIGHT - 2, BOARD_LABEL_X, "   a      b      c      d      e      f      g      h");
    wrefresh(board_win);
}

// 채팅 영역 그리기
void draw_chat_area(WINDOW *chat_win)
{
    client_state_t *client = get_client_state();

    werase(chat_win);
    draw_border(chat_win);

    mvwprintw(chat_win, 1, 2, "Chat");

    // 채팅 메시지 표시
    game_state_t *state = &client->game_state;
    int display_count = (state->chat_count > 12) ? 12 : state->chat_count;
    int start_index = (state->chat_count > 12) ? state->chat_count - 12 : 0;

    for (int i = 0; i < display_count; i++)
    {
        chat_message_t *msg = &state->chat_messages[start_index + i];
        mvwprintw(chat_win, 3 + i, 2, "%s: %s", msg->sender, msg->message);
    }

    wrefresh(chat_win);
}

// 게임 메뉴 그리기
void draw_game_menu(WINDOW *menu_win)
{
    client_state_t *client = get_client_state();

    werase(menu_win);
    draw_border(menu_win);

    mvwprintw(menu_win, 1, 2, "Game Menu");
    mvwprintw(menu_win, 3, 2, "1. Resign");
    mvwprintw(menu_win, 4, 2, "2. Draw");
    mvwprintw(menu_win, 5, 2, "3. Save");
    mvwprintw(menu_win, 6, 2, "4. Main");

    // 현재 턴 표시
    board_state_t *board = &client->game_state.board_state;
    const char *turn_text = (board->current_turn == TEAM_WHITE) ? "White" : "Black";
    mvwprintw(menu_win, 8, 2, "Turn: %s", turn_text);

    wrefresh(menu_win);
}

// 게임 화면 그리기
void draw_game_screen()
{
    client_state_t *client = get_client_state();

    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW *board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, 1, 2);
    draw_chess_board(board_win);

    WINDOW *chat_win = newwin(CHAT_HEIGHT, CHAT_WIDTH, 1, BOARD_WIDTH + 5);
    draw_chat_area(chat_win);

    WINDOW *input_win = newwin(INPUT_HEIGHT, CHAT_WIDTH, CHAT_HEIGHT + 2, BOARD_WIDTH + 5);
    draw_border(input_win);
    mvwprintw(input_win, 1, 2, "Message: (Enter)");
    wrefresh(input_win);

    WINDOW *menu_win = newwin(10, 20, BOARD_HEIGHT + 2, 2);
    draw_game_menu(menu_win);

    mvprintw(0, 2, "Player: %s vs %s", client->username, client->opponent_name);
    mvprintw(0, cols - 20, "Status: Playing");

    wrefresh(board_win);
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(menu_win);
    refresh();
}

// 매칭 타이머 업데이트 (매초 호출)
void update_match_timer()
{
    client_state_t *client = get_client_state();

    if (client->current_screen == SCREEN_MATCHING)
    {
        // 타임아웃 체크 (30초)
        time_t current_time = time(NULL);
        if (current_time - client->match_start_time >= 30)
        {
            add_chat_message_safe("System", "Matchmaking timeout");
            client->current_screen = SCREEN_MAIN;
        }
    }
}