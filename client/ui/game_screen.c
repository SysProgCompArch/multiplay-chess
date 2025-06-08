#include <string.h>
#include <time.h>

#include "../client_state.h"
#include "piece.h"
#include "rule.h"
#include "ui.h"

// 체스 말 유니코드 문자 반환
const char *get_piece_unicode(piece_t *piece, color_t color) {
    if (!piece)
        return " ";

    // 흰색 팀 (0) - 흰색 유니코드, 검은색 팀 (1) - 검은색 유니코드
    if (color == TEAM_WHITE) {  // 흰색
        switch (piece->type) {
            case PIECE_KING:
                return "W KNG";
            case PIECE_QUEEN:
                return "W QUE";
            case PIECE_ROOK:
                return "W ROK";
            case PIECE_BISHOP:
                return "W BIS";
            case PIECE_KNIGHT:
                return "W KNT";
            case PIECE_PAWN:
                return "W PAW";
        }
    } else {  // 검은색
        switch (piece->type) {
            case PIECE_KING:
                return "B KNG";
            case PIECE_QUEEN:
                return "B QUE";
            case PIECE_ROOK:
                return "B ROK";
            case PIECE_BISHOP:
                return "B BIS";
            case PIECE_KNIGHT:
                return "B KNT";
            case PIECE_PAWN:
                return "B PAW";
        }
    }
    return " ";
}

// 선택된 기물의 이동 가능한 위치들을 계산하는 함수
void calculate_possible_moves(game_state_t *game_state, int selected_x, int selected_y, bool piece_selected) {
    // 배열 초기화
    memset(game_state->possible_moves, false, sizeof(game_state->possible_moves));
    memset(game_state->capture_moves, false, sizeof(game_state->capture_moves));

    if (!piece_selected) {
        return;
    }

    // game_t를 직접 사용
    game_t *game = &game_state->game;

    int sx = selected_x;
    int sy = selected_y;

    // 선택된 위치에 기물이 있는지 확인
    piecestate_t *selected_piece = &game->board[sy][sx];
    if (!selected_piece->piece || selected_piece->is_dead) {
        return;
    }

    // 모든 위치에 대해 이동 가능한지 검사
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            if (sx == dx && sy == dy) {
                continue;  // 같은 위치는 제외
            }

            // common/rule.c의 is_move_legal 함수 사용
            if (is_move_legal(game, sx, sy, dx, dy)) {
                piecestate_t *target_piece = &game->board[dy][dx];

                // 목표 위치에 상대 기물이 있으면 캡처 가능 위치로 표시
                if (target_piece->piece && !target_piece->is_dead &&
                    target_piece->color != selected_piece->color) {
                    game_state->capture_moves[dy][dx] = true;
                } else {
                    game_state->possible_moves[dy][dx] = true;
                }
            }
        }
    }
}

// 체스 보드 그리기
void draw_chess_board(WINDOW *board_win) {
    client_state_t *client = get_client_state();

    werase(board_win);
    draw_border(board_win);

    // 플레이어 관점에 따른 라벨 및 좌표 설정
    bool is_black_player = (client->game_state.local_team == TEAM_BLACK);

    if (is_black_player) {
        // 검은색 플레이어: h-a 순서 (뒤집힌 표시)
        mvwprintw(board_win, 1, BOARD_LABEL_Y, "     h      g      f      e      d      c      b      a");
    } else {
        // 흰색 플레이어: a-h 순서 (표준 표시)
        mvwprintw(board_win, 1, BOARD_LABEL_Y, "     a      b      c      d      e      f      g      h");
    }

    game_t *game = &client->game_state.game;

    for (int row = 0; row < BOARD_SIZE; row++) {
        // 플레이어 관점에 따른 실제 보드 좌표 계산
        int actual_row   = is_black_player ? row : (7 - row);
        int display_rank = is_black_player ? (row + 1) : (8 - row);

        // 왼쪽 라벨: 테두리 다음 칸(1)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, 1, "%d ", display_rank);

        for (int col = 0; col < BOARD_SIZE; col++) {
            int actual_col = is_black_player ? (7 - col) : col;

            int  y           = 2 + row * SQUARE_H;
            int  x           = BOARD_LABEL_X + col * SQUARE_W;
            bool is_selected = (client->piece_selected &&
                                client->selected_x == actual_col &&
                                client->selected_y == actual_row);

            // 커서 위치 확인 (display 좌표와 직접 비교)
            int cursor_x, cursor_y;
            get_cursor_position(&cursor_x, &cursor_y);
            bool is_cursor = (is_cursor_mode() && cursor_x == col && cursor_y == row);

            // 이동 가능 위치 확인 (실제 좌표 사용)
            bool is_possible_move = client->game_state.possible_moves[actual_row][actual_col];
            bool is_capture_move  = client->game_state.capture_moves[actual_row][actual_col];

            // 색상 (우선순위: 선택됨 > 커서 > 캡처 > 이동가능 > 기본)
            if (is_selected) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_SELECTED_SQUARE));
            } else if (is_cursor) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_CURSOR_SQUARE));
            } else if (is_capture_move) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_CAPTURE_MOVE));
            } else if (is_possible_move) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_POSSIBLE_MOVE));
            } else if ((row + col) % 2 == 0) {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            } else {
                wattron(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            }
            // 3줄로 한 칸 출력
            mvwprintw(board_win, y, x, "       ");
            piecestate_t *piece = &game->board[actual_row][actual_col];
            if (piece->piece && !piece->is_dead) {
                const char *piece_unicode = get_piece_unicode(piece->piece, piece->color);

                // 기물이 내 기물인지 상대방 기물인지 판단
                team_t piece_team  = piece->color;
                bool   is_my_piece = (piece_team == client->game_state.local_team);

                if (is_my_piece) {
                    // 내 기물: 볼드 속성만 추가 (배경색은 기존 칸 색상 유지)
                    wattron(board_win, A_BOLD);
                    mvwprintw(board_win, y + 1, x, " %s ", piece_unicode);
                    wattroff(board_win, A_BOLD);
                } else {
                    // 상대방 기물: 어둡게 표시 (배경색은 기존 칸 색상 유지)
                    wattron(board_win, A_DIM);
                    mvwprintw(board_win, y + 1, x, " %s ", piece_unicode);
                    wattroff(board_win, A_DIM);
                }
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
            } else if (is_capture_move) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_CAPTURE_MOVE));
            } else if (is_possible_move) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_POSSIBLE_MOVE));
            } else if ((row + col) % 2 == 0) {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_WHITE));
            } else {
                wattroff(board_win, COLOR_PAIR(COLOR_PAIR_BOARD_BLACK));
            }
        }
        // 오른쪽 라벨: 테두리 바로 안쪽(BOARD_WIDTH - 2)에 표시
        mvwprintw(board_win, 2 + row * SQUARE_H + 1, BOARD_WIDTH - 2, "%d", display_rank);
    }

    // 하단 라벨 (플레이어 관점에 따라)
    if (is_black_player) {
        mvwprintw(board_win, BOARD_HEIGHT - 2, BOARD_LABEL_X, "   h      g      f      e      d      c      b      a");
    } else {
        mvwprintw(board_win, BOARD_HEIGHT - 2, BOARD_LABEL_X, "   a      b      c      d      e      f      g      h");
    }

    wrefresh(board_win);
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

    int current_line      = 3;                        // 메시지 표시 시작 라인
    int max_line          = win_height - 2;           // 아래 테두리 바로 위까지
    int max_display_lines = max_line - current_line;  // 실제 사용 가능한 라인 수
    if (max_display_lines < 1) max_display_lines = 1;

    // 채팅 메시지 표시
    game_state_t *state = &client->game_state;

    // 역순으로 메시지를 처리하여 최신 메시지가 아래쪽에 표시되도록 함
    int total_lines_needed = 0;

    // 먼저 필요한 총 라인 수를 계산
    for (int i = state->chat_count - 1; i >= 0; i--) {
        chat_message_t *msg = &state->chat_messages[i];
        char            full_msg[512];
        snprintf(full_msg, sizeof(full_msg), "%s: %s", msg->sender, msg->message);

        int max_line_len = win_width - 4;  // 테두리와 여백 제외
        int msg_len      = strlen(full_msg);

        if (msg_len <= max_line_len) {
            total_lines_needed += 1;
        } else {
            // 여러 줄 메시지의 라인 수 계산 (들여쓰기 고려)
            int indent = strlen(msg->sender) + 2;        // "sender: " 길이만큼 들여쓰기
            if (indent > max_line_len - 10) indent = 4;  // 너무 긴 이름의 경우 기본 들여쓰기

            int first_line_len = max_line_len;
            int remaining_len  = msg_len - first_line_len;
            int lines_for_msg  = 1;  // 첫 줄

            // 나머지 줄들 (들여쓰기 적용)
            if (remaining_len > 0) {
                int subsequent_line_len = max_line_len - indent;
                if (subsequent_line_len < 10) subsequent_line_len = 10;  // 최소 길이 보장
                lines_for_msg += (remaining_len + subsequent_line_len - 1) / subsequent_line_len;
            }

            total_lines_needed += lines_for_msg;
        }

        if (total_lines_needed >= max_display_lines) {
            break;
        }
    }

    // 표시할 메시지들을 다시 순서대로 출력
    int start_msg_index = 0;
    int lines_counted   = 0;

    // 표시할 첫 번째 메시지 인덱스 찾기
    for (int i = state->chat_count - 1; i >= 0; i--) {
        chat_message_t *msg = &state->chat_messages[i];
        char            full_msg[512];
        snprintf(full_msg, sizeof(full_msg), "%s: %s", msg->sender, msg->message);

        int max_line_len = win_width - 4;
        int msg_len      = strlen(full_msg);

        int lines_for_this_msg;
        if (msg_len <= max_line_len) {
            lines_for_this_msg = 1;
        } else {
            // 여러 줄 메시지의 라인 수 계산 (들여쓰기 고려)
            int indent = strlen(msg->sender) + 2;
            if (indent > max_line_len - 10) indent = 4;

            int first_line_len = max_line_len;
            int remaining_len  = msg_len - first_line_len;
            lines_for_this_msg = 1;  // 첫 줄

            // 나머지 줄들 (들여쓰기 적용)
            if (remaining_len > 0) {
                int subsequent_line_len = max_line_len - indent;
                if (subsequent_line_len < 10) subsequent_line_len = 10;
                lines_for_this_msg += (remaining_len + subsequent_line_len - 1) / subsequent_line_len;
            }
        }

        if (lines_counted + lines_for_this_msg > max_display_lines) {
            start_msg_index = i + 1;
            break;
        }

        lines_counted += lines_for_this_msg;
        start_msg_index = i;
    }

    // 실제 메시지 출력
    for (int i = start_msg_index; i < state->chat_count && current_line < max_line; i++) {
        chat_message_t *msg = &state->chat_messages[i];

        // 전체 메시지 준비
        char full_msg[512];
        snprintf(full_msg, sizeof(full_msg), "%s: %s", msg->sender, msg->message);

        int max_line_len = win_width - 4;  // 테두리와 여백 제외
        int msg_len      = strlen(full_msg);

        // 메시지가 한 줄에 들어가는 경우
        if (msg_len <= max_line_len) {
            if (current_line < max_line) {
                mvwprintw(chat_win, current_line, 2, "%s", full_msg);
                current_line++;
            }
        } else {
            // 메시지가 여러 줄에 걸치는 경우
            int  pos        = 0;
            bool first_line = true;

            while (pos < msg_len && current_line < max_line) {
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

    wrefresh(chat_win);
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
    game_t     *game      = &client->game_state.game;
    const char *turn_text = (game->side_to_move == TEAM_WHITE) ? "White" : "Black";
    mvwprintw(menu_win, 8, 2, "Turn: %s", turn_text);

    wrefresh(menu_win);
}

// 플레이어 정보 표시
void draw_player_info(WINDOW *player_win, const char *player_name, bool is_me, team_t team, bool is_current_turn, int time_remaining, bool is_in_check) {
    werase(player_win);

    // 체크 상태에 따른 색상 설정
    int color_pair;
    if (is_in_check) {
        color_pair = COLOR_PAIR_CHECK_PLAYER;  // 체크 상태: 붉은 배경, 흰 글자
    } else {
        color_pair = is_current_turn ? COLOR_PAIR_WHITE_PLAYER : COLOR_PAIR_BLACK_PLAYER;
    }

    wattron(player_win, COLOR_PAIR(color_pair));

    // 전체 윈도우를 배경색으로 채우기
    int height, width;
    getmaxyx(player_win, height, width);

    for (int i = 0; i < height; i++) {
        mvwprintw(player_win, i, 0, "%*s", width, " ");
    }

    // 플레이어 이름 표시 (왼쪽)
    char        display_name[64];
    const char *team_text = (team == TEAM_WHITE) ? "[WHITE]" : "[BLACK]";

    if (is_me) {
        snprintf(display_name, sizeof(display_name), "%s %s (ME)", team_text, player_name);
    } else {
        snprintf(display_name, sizeof(display_name), "%s %s", team_text, player_name);
    }

    // 현재 턴이면 강조 표시
    if (is_current_turn) {
        wattron(player_win, A_BOLD);
        mvwprintw(player_win, 0, 1, "► %s", display_name);
        wattroff(player_win, A_BOLD);
    } else {
        mvwprintw(player_win, 0, 1, "  %s", display_name);
    }

    // 체크 상태일 때 CHECK 텍스트 표시
    if (is_in_check) {
        // 타이머 표시 위치 계산
        int  minutes = time_remaining / 60;
        int  seconds = time_remaining % 60;
        char timer_str[16];
        snprintf(timer_str, sizeof(timer_str), "%02d:%02d", minutes, seconds);

        int timer_x = width - strlen(timer_str) - 1;
        int check_x = timer_x - strlen("CHECK") - 1;  // CHECK + 공백

        // CHECK 표시 (볼드체)
        if (check_x > strlen(display_name) + 4) {
            wattron(player_win, A_BOLD);
            mvwprintw(player_win, 0, check_x, "CHECK ");
            wattroff(player_win, A_BOLD);
        }

        // 타이머 표시
        mvwprintw(player_win, 0, timer_x, "%s", timer_str);
    } else {
        // 체크 상태가 아닐 때는 타이머만 표시
        int  minutes = time_remaining / 60;
        int  seconds = time_remaining % 60;
        char timer_str[16];
        snprintf(timer_str, sizeof(timer_str), "%02d:%02d", minutes, seconds);

        int timer_x = width - strlen(timer_str) - 1;
        if (timer_x > strlen(display_name) + 4) {
            mvwprintw(player_win, 0, timer_x, "%s", timer_str);
        }
    }

    wattroff(player_win, COLOR_PAIR(color_pair));
    wrefresh(player_win);
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

// 게임 화면 그리기
void draw_game_screen() {
    client_state_t *client = get_client_state();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // 동적 채팅창 크기 계산
    int chat_width, input_width;
    calculate_chat_dimensions(cols, &chat_width, &input_width);

    // 플레이어 정보 및 체스판 위치 계산
    int player_info_height = 1;                       // 플레이어 정보창 높이
    int board_start_y      = 1 + player_info_height;  // 상대방 정보창 + 간격

    // 상대방 정보창 (체스판 위)
    game_state_t *game_state    = &client->game_state;
    team_t        opponent_team = (game_state->local_team == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;
    bool          opponent_turn = (game_state->game.side_to_move == opponent_team);
    int           opponent_time = (opponent_team == TEAM_WHITE) ? game_state->white_time_remaining : game_state->black_time_remaining;

    WINDOW *opponent_info_win = newwin(player_info_height, BOARD_WIDTH, 1, 2);
    bool    opponent_in_check = (opponent_team == TEAM_WHITE) ? game_state->white_in_check : game_state->black_in_check;
    draw_player_info(opponent_info_win, get_opponent_name_client(), false, opponent_team, opponent_turn, opponent_time, opponent_in_check);

    // 체스판 (가운데)
    WINDOW *board_win = newwin(BOARD_HEIGHT, BOARD_WIDTH, board_start_y, 2);

    // 이동 가능한 위치 계산
    calculate_possible_moves(&client->game_state, client->selected_x, client->selected_y, client->piece_selected);

    draw_chess_board(board_win);

    // 내 정보창 (체스판 아래)
    bool my_turn = (game_state->game.side_to_move == game_state->local_team);
    int  my_time = (game_state->local_team == TEAM_WHITE) ? game_state->white_time_remaining : game_state->black_time_remaining;

    WINDOW *my_info_win = newwin(player_info_height, BOARD_WIDTH, board_start_y + BOARD_HEIGHT, 2);
    bool    my_in_check = (game_state->local_team == TEAM_WHITE) ? game_state->white_in_check : game_state->black_in_check;
    draw_player_info(my_info_win, client->username, true, game_state->local_team, my_turn, my_time, my_in_check);

    // 채팅창 높이를 체스판과 맞춤 (플레이어 정보창 2개 + 체스판 높이)
    int     total_game_height = player_info_height * 2 + BOARD_HEIGHT;
    int     chat_start_x      = 2 + BOARD_WIDTH + 1;                                         // 왼쪽여백 + 체스판 + 중간여백(1)
    WINDOW *chat_win          = newwin(total_game_height - 2, chat_width, 2, chat_start_x);  // 위아래 한칸씩 줄임
    draw_chat_area(chat_win);

    // 입력창은 채팅창 아래에 위치
    WINDOW *input_win = newwin(INPUT_HEIGHT, input_width, 2 + (total_game_height - 2), chat_start_x);
    draw_chat_input(input_win);

    wrefresh(opponent_info_win);
    wrefresh(board_win);
    wrefresh(my_info_win);
    wrefresh(chat_win);
    wrefresh(input_win);

    // 조작법 안내 (화면 하단)
    int help_y = rows - 2;
    mvprintw(help_y, 2, "Controls: Arrow keys + Space/Enter | ESC: cancel | Q: quit | Enter: chat | move:e2e4");

    // 연결 상태 표시
    draw_connection_status();
}