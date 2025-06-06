#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "../client_state.h"
#include "piece.h"
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

// 메인 화면 그리기
void draw_main_screen() {
    clear();
    wnoutrefresh(stdscr);  // stdscr 변경사항을 가상 화면에 반영

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 더 큰 메인 윈도우 크기
    int main_height = 25;
    int main_width  = 80;
    int start_y     = (rows - main_height) / 2;
    int start_x     = (cols - main_width) / 2;

    WINDOW *main_win = newwin(main_height, main_width, start_y, start_x);

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
    mvwprintw(main_win, 16, (main_width - 18) / 2, "2. Options       ");
    mvwprintw(main_win, 18, (main_width - 18) / 2, "3. Exit          ");
    wattroff(main_win, A_BOLD);

    // 하단 장식
    wattron(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));
    mvwprintw(main_win, 21, (main_width - 60) / 2, "═══════════════════════════════════════════════════════════");
    wattroff(main_win, COLOR_PAIR(COLOR_PAIR_BORDER));

    wnoutrefresh(main_win);

    // 연결 상태 표시
    draw_connection_status();
    wnoutrefresh(stdscr);  // stdscr 변경사항도 가상 화면에 반영

    // 모든 그리기 작업 완료 후 한 번에 화면 업데이트
    doupdate();
}

// 매칭 화면 그리기
void draw_matching_screen() {
    clear();
    wnoutrefresh(stdscr);  // stdscr 변경사항을 가상 화면에 반영

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - MATCH_DIALOG_HEIGHT) / 2;
    int start_x = (cols - MATCH_DIALOG_WIDTH) / 2;

    WINDOW *match_win = newwin(MATCH_DIALOG_HEIGHT, MATCH_DIALOG_WIDTH, start_y, start_x);
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

    wnoutrefresh(match_win);

    // 연결 상태 표시
    draw_connection_status();
    wnoutrefresh(stdscr);  // stdscr 변경사항도 가상 화면에 반영

    // 모든 그리기 작업 완료 후 한 번에 화면 업데이트
    doupdate();
}

// 체스 말 ASCII 문자 반환
char get_piece_char(piece_t *piece, int team) {
    if (!piece)
        return ' ';

    // 흰색 팀 (0) - 대문자, 검은색 팀 (1) - 소문자
    if (team == 0) {  // 흰색
        switch (piece->type) {
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
    } else {  // 검은색
        switch (piece->type) {
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
void draw_chess_board(WINDOW *board_win) {
    client_state_t *client = get_client_state();

    werase(board_win);
    draw_border(board_win);

    // 상단 라벨 (테두리+라벨)
    mvwprintw(board_win, 1, BOARD_LABEL_Y, "     a      b      c      d      e      f      g      h");

    board_state_t *board = &client->game_state.board_state;

    for (int row = 0; row < BOARD_SIZE; row++) {
        // 왼쪽 라벨: 테두리 다음 칸(1)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, 1, "%d ", 8 - row);
        for (int col = 0; col < BOARD_SIZE; col++) {
            int  y           = 2 + row * SQUARE_H;
            int  x           = BOARD_LABEL_X + col * SQUARE_W;
            bool is_selected = (client->piece_selected &&
                                client->selected_x == col &&
                                client->selected_y == row);

            // 커서 위치 확인
            int cursor_x, cursor_y;
            get_cursor_position(&cursor_x, &cursor_y);
            bool is_cursor = (is_cursor_mode() && cursor_x == col && cursor_y == row);

            // 색상
            if (is_selected) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            } else if (is_cursor) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_CURSOR_SQUARE));
            } else if ((row + col) % 2 == 0) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            } else {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            // 3줄로 한 칸 출력
            mvwprintw(board_win, y, x, "       ");
            piecestate_t *piece = &board->board[row][col];
            if (piece->piece && !piece->is_dead) {
                char piece_char = get_piece_char(piece->piece, 0);  // 임시로 흰색
                mvwprintw(board_win, y + 1, x, "   %c   ", piece_char);
            } else if (is_cursor) {
                mvwprintw(board_win, y + 1, x, "   *   ");
            } else {
                mvwprintw(board_win, y + 1, x, "       ");
            }
            mvwprintw(board_win, y + 2, x, "       ");
            // 색상 해제
            if (is_selected) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            } else if (is_cursor) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_CURSOR_SQUARE));
            } else if ((row + col) % 2 == 0) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            } else {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
        }
        // 오른쪽 라벨: 테두리 바로 안쪽(BOARD_WIDTH - 2)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, BOARD_WIDTH - 2, "%d", 8 - row);
    }
    // 하단 라벨
    mvwprintw(board_win, BOARD_HEIGHT - 2, BOARD_LABEL_X, "   a      b      c      d      e      f      g      h");
    wnoutrefresh(board_win);
}

// 채팅 영역 그리기
void draw_chat_area(WINDOW *chat_win) {
    client_state_t *client = get_client_state();

    werase(chat_win);
    draw_border(chat_win);

    mvwprintw(chat_win, 1, 2, "Chat");

    // 채팅창 크기에 맞춰 표시할 메시지 수 계산
    int win_height, win_width;
    getmaxyx(chat_win, win_height, win_width);

    int max_messages = win_height - 4;  // 테두리와 제목 제외
    if (max_messages < 1) max_messages = 1;

    // 채팅 메시지 표시
    game_state_t *state         = &client->game_state;
    int           display_count = (state->chat_count > max_messages) ? max_messages : state->chat_count;
    int           start_index   = (state->chat_count > max_messages) ? state->chat_count - max_messages : 0;

    int current_line = 3;  // 메시지 표시 시작 라인

    for (int i = 0; i < display_count && current_line < win_height - 1; i++) {
        chat_message_t *msg = &state->chat_messages[start_index + i];

        // 전체 메시지 준비
        char full_msg[512];
        snprintf(full_msg, sizeof(full_msg), "%s: %s", msg->sender, msg->message);

        int max_line_len = win_width - 4;  // 테두리와 여백 제외
        int msg_len      = strlen(full_msg);

        // 메시지가 한 줄에 들어가는 경우
        if (msg_len <= max_line_len) {
            mvwprintw(chat_win, current_line, 2, "%s", full_msg);
            current_line++;
        } else {
            // 메시지가 여러 줄에 걸치는 경우
            int  pos        = 0;
            bool first_line = true;

            while (pos < msg_len && current_line < win_height - 1) {
                char line_buffer[256];
                int  copy_len = (msg_len - pos > max_line_len) ? max_line_len : msg_len - pos;

                // 첫 줄이 아닌 경우 들여쓰기 적용
                if (!first_line) {
                    int indent = strlen(msg->sender) + 2;        // "sender: " 길이만큼 들여쓰기
                    if (indent > max_line_len - 10) indent = 4;  // 너무 긴 이름의 경우 기본 들여쓰기

                    for (int j = 0; j < indent && j < max_line_len - copy_len; j++) {
                        line_buffer[j] = ' ';
                    }
                    strncpy(line_buffer + indent, full_msg + pos, copy_len);
                    line_buffer[indent + copy_len] = '\0';
                } else {
                    strncpy(line_buffer, full_msg + pos, copy_len);
                    line_buffer[copy_len] = '\0';
                }

                mvwprintw(chat_win, current_line, 2, "%s", line_buffer);

                pos += copy_len;
                current_line++;
                first_line = false;
            }
        }
    }

    wnoutrefresh(chat_win);
}

// 게임 메뉴 그리기
void draw_game_menu(WINDOW *menu_win) {
    client_state_t *client = get_client_state();

    werase(menu_win);
    draw_border(menu_win);

    mvwprintw(menu_win, 1, 2, "Game Menu");
    mvwprintw(menu_win, 3, 2, "1. Resign");
    mvwprintw(menu_win, 4, 2, "2. Draw");
    mvwprintw(menu_win, 5, 2, "3. Save");
    mvwprintw(menu_win, 6, 2, "Q. Quit to Main");

    // 현재 턴 표시
    board_state_t *board     = &client->game_state.board_state;
    const char    *turn_text = (board->current_turn == TEAM_WHITE) ? "White" : "Black";
    mvwprintw(menu_win, 8, 2, "Turn: %s", turn_text);

    wnoutrefresh(menu_win);
}

// 게임 화면 그리기
void draw_game_screen() {
    client_state_t *client = get_client_state();

    // 전체 화면을 지우되, 바로 refresh하지 않음
    clear();
    wnoutrefresh(stdscr);  // stdscr 변경사항을 가상 화면에 반영

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 동적 채팅창 크기 계산
    int chat_width, input_width;
    calculate_chat_dimensions(cols, &chat_width, &input_width);

    // 플레이어 정보 및 체스판 위치 계산
    int player_info_height = 1;                       // 플레이어 정보창 높이
    int board_start_y      = 1 + player_info_height;  // 상대방 정보창 + 간격

    // 상대방 정보창 (체스판 위)
    game_state_t *game          = &client->game_state;
    team_t        opponent_team = (game->local_team == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
    bool          opponent_turn = (game->board_state.current_turn == opponent_team);
    int           opponent_time = (opponent_team == TEAM_WHITE) ? game->white_time_remaining : game->black_time_remaining;

    WINDOW *opponent_info_win = newwin(player_info_height, BOARD_WIDTH, 1, 2);
    draw_player_info(opponent_info_win, get_opponent_name_client(), false, opponent_team, opponent_turn, opponent_time);

    // 체스판 (가운데)
    WINDOW *board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, board_start_y, 2);
    draw_chess_board(board_win);

    // 내 정보창 (체스판 아래)
    bool my_turn = (game->board_state.current_turn == game->local_team);
    int  my_time = (game->local_team == TEAM_WHITE) ? game->white_time_remaining : game->black_time_remaining;

    WINDOW *my_info_win = newwin(player_info_height, BOARD_WIDTH, board_start_y + BOARD_HEIGHT, 2);
    draw_player_info(my_info_win, client->username, true, game->local_team, my_turn, my_time);

    // 채팅창 높이를 체스판과 맞춤 (플레이어 정보창 2개 + 체스판 높이)
    int     total_game_height = player_info_height * 2 + BOARD_HEIGHT;
    int     chat_start_x      = 2 + BOARD_WIDTH + 1;                                         // 왼쪽여백 + 체스판 + 중간여백(1)
    WINDOW *chat_win          = newwin(total_game_height - 2, chat_width, 2, chat_start_x);  // 위아래 한칸씩 줄임
    draw_chat_area(chat_win);

    // 입력창은 채팅창 아래에 위치
    WINDOW *input_win = newwin(INPUT_HEIGHT, input_width, 2 + (total_game_height - 2), chat_start_x);
    draw_chat_input(input_win);

    // 게임 메뉴는 내 정보창 아래에 위치
    // WINDOW *menu_win = newwin(10, 20, board_start_y + BOARD_HEIGHT + player_info_height, 2);
    // draw_game_menu(menu_win);

    wnoutrefresh(opponent_info_win);
    wnoutrefresh(board_win);
    wnoutrefresh(my_info_win);
    wnoutrefresh(chat_win);
    wnoutrefresh(input_win);
    // wnoutrefresh(menu_win);

    // 조작법 안내 (화면 하단)
    int help_y = rows - 2;
    mvprintw(help_y, 2, "Controls: Arrow keys + Space/Enter | ESC to cancel | Q to quit | Enter to chat | move:e2e4");

    // 연결 상태 표시
    draw_connection_status();
    wnoutrefresh(stdscr);  // stdscr 변경사항도 가상 화면에 반영

    // 모든 그리기 작업 완료 후 한 번에 화면 업데이트
    doupdate();
}

// 플레이어 정보 표시
void draw_player_info(WINDOW *player_win, const char *player_name, bool is_me, team_t team, bool is_current_turn, int time_remaining) {
    werase(player_win);

    // 플레이어 팀에 따른 색상 설정
    int color_pair = (team == TEAM_WHITE) ? COLOR_PAIR_WHITE_PLAYER : COLOR_PAIR_BLACK_PLAYER;
    wattron(player_win, COLOR_PAIR(color_pair));

    // 전체 윈도우를 팀 색상으로 채우기
    int height, width;
    getmaxyx(player_win, height, width);

    for (int i = 0; i < height; i++) {
        mvwprintw(player_win, i, 0, "%*s", width, " ");
    }

    // 플레이어 이름 표시 (왼쪽)
    char display_name[64];
    if (is_me) {
        snprintf(display_name, sizeof(display_name), "%s (ME)", player_name);
    } else {
        snprintf(display_name, sizeof(display_name), "%s", player_name);
    }

    // 현재 턴이면 강조 표시
    if (is_current_turn) {
        wattron(player_win, A_BOLD);
        mvwprintw(player_win, 0, 1, "► %s", display_name);
        wattroff(player_win, A_BOLD);
    } else {
        mvwprintw(player_win, 0, 1, "  %s", display_name);
    }

    // 타이머 표시 (오른쪽 정렬)
    int  minutes = time_remaining / 60;
    int  seconds = time_remaining % 60;
    char timer_str[16];
    snprintf(timer_str, sizeof(timer_str), "%02d:%02d", minutes, seconds);

    int timer_x = width - strlen(timer_str) - 1;
    if (timer_x > strlen(display_name) + 4) {  // 이름과 겹치지 않도록
        mvwprintw(player_win, 0, timer_x, "%s", timer_str);
    }

    wattroff(player_win, COLOR_PAIR(color_pair));
    wnoutrefresh(player_win);
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

// 채팅창 크기 동적 계산
void calculate_chat_dimensions(int terminal_cols, int *chat_width, int *input_width) {
    // 체스판 크기는 고정 (BOARD_WIDTH = 60)
    // 왼쪽 여백(2) + 체스판(60) + 중간 여백(1) + 채팅창 + 오른쪽 여백(2) = 터미널 너비
    // 따라서 채팅창 사용 가능 너비 = 터미널 너비 - 65

    int available_width = terminal_cols - BOARD_WIDTH - 2 - 1 - 2;  // 왼쪽여백 + 체스판 + 중간여백 + 오른쪽여백

    // 채팅창 최소 크기 35, 최대 크기 80
    if (available_width < 35) {
        *chat_width = 35;
    } else if (available_width > 80) {
        *chat_width = 80;
    } else {
        *chat_width = available_width;
    }

    *input_width = *chat_width;  // 입력창도 같은 크기
}