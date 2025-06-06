#include <string.h>

#include "../client_state.h"
#include "piece.h"
#include "ui.h"

// 화면 좌표를 보드 좌표로 변환
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y) {
    // 보드 윈도우 시작 위치 (draw_game_screen에서 newwin 위치)
    int board_win_start_x = 2;
    int board_win_start_y = 1;

    // 윈도우 내에서의 상대 좌표 계산
    int rel_x = screen_x - board_win_start_x;
    int rel_y = screen_y - board_win_start_y;

    // 체스판 시작 위치 (윈도우 내에서 BOARD_LABEL_X, 2 오프셋)
    int chess_start_x = BOARD_LABEL_X;
    int chess_start_y = 2;

    // 체스판 영역 내인지 확인
    if (rel_x < chess_start_x || rel_y < chess_start_y)
        return false;

    // 체스판 내 상대 좌표
    int chess_rel_x = rel_x - chess_start_x;
    int chess_rel_y = rel_y - chess_start_y;

    // 체스판 전체 크기 체크
    int total_chess_width  = SQUARE_W * BOARD_SIZE;
    int total_chess_height = SQUARE_H * BOARD_SIZE;

    if (chess_rel_x >= total_chess_width || chess_rel_y >= total_chess_height)
        return false;

    // 어느 칸인지 계산
    int display_x = chess_rel_x / SQUARE_W;
    int display_y = chess_rel_y / SQUARE_H;

    // 범위 확인
    if (display_x >= 0 && display_x < BOARD_SIZE && display_y >= 0 && display_y < BOARD_SIZE) {
        // 블랙 플레이어인지 확인하여 좌표 변환
        client_state_t *client          = get_client_state();
        bool            is_black_player = (client->game_state.local_team == TEAM_BLACK);

        if (is_black_player) {
            *board_x = 7 - display_x;
            *board_y = 7 - display_y;
        } else {
            *board_x = display_x;
            *board_y = display_y;
        }
        return true;
    }

    return false;
}

// 보드 클릭 처리
void handle_board_click(int board_x, int board_y) {
    client_state_t *client = get_client_state();

    if (client->current_screen != SCREEN_GAME)
        return;

    board_state_t *board = &client->game_state.board_state;

    // 내부 로직용 디버그 메시지
    char debug_msg[256];
    snprintf(debug_msg, sizeof(debug_msg), "보드 클릭: (%d, %d) - 이미 변환된 좌표", board_x, board_y);
    add_chat_message_safe("System", debug_msg);

    if (!client->piece_selected) {
        // 기물 선택
        piecestate_t *piece = &board->board[board_y][board_x];
        if (piece->piece && !piece->is_dead) {
            // 현재 플레이어의 기물인지 확인
            team_t piece_team = (piece->color == 0) ? TEAM_WHITE : TEAM_BLACK;

            // 디버그 메시지 추가
            char debug_msg[256];
            snprintf(debug_msg, sizeof(debug_msg), "DEBUG: piece_color=%d, piece_team=%d, local_team=%d, current_turn=%d, piece_y=%d, piece_x=%d",
                     piece->color, piece_team, client->game_state.local_team, board->current_turn, board_y, board_x);
            add_chat_message_safe("System", debug_msg);

            // 보드 출력 디버그 - 모든 행 확인
            for (int i = 0; i < 8; i++) {
                char row_debug[256];
                snprintf(row_debug, sizeof(row_debug), "Board[%d]: %d %d %d %d %d %d %d %d",
                         i, board->board[i][0].color, board->board[i][1].color,
                         board->board[i][2].color, board->board[i][3].color,
                         board->board[i][4].color, board->board[i][5].color,
                         board->board[i][6].color, board->board[i][7].color);
                add_chat_message_safe("System", row_debug);
            }

            // 블랙 플레이어인 경우 기물 색상 매핑을 다르게 해야 함
            bool is_black_player = (client->game_state.local_team == TEAM_BLACK);

            // 자신의 기물인지 확인 - 원래는 로컬 팀과 기물 색상이 일치해야 함
            // 그러나 실제 보드에서는 color 값이 정확히 일치하지 않을 수 있음
            if ((is_black_player && piece->color != BLACK) || (!is_black_player && piece->color != WHITE)) {
                add_chat_message_safe("System", "You can only move your own pieces!");
                return;
            }

            // 현재 턴인지 확인
            if (board->current_turn != client->game_state.local_team) {
                char turn_msg[128];
                snprintf(turn_msg, sizeof(turn_msg), "It's not your turn! current_turn=%d, local_team=%d",
                         board->current_turn, client->game_state.local_team);
                add_chat_message_safe("System", turn_msg);
                return;
            }

            // 턴 확인 완료 - 자신의 기물을 선택함

            client->selected_x     = board_x;
            client->selected_y     = board_y;
            client->piece_selected = true;

            add_chat_message_safe("System", "Piece selected. Click destination.");
        }
    } else {
        // 기물 이동
        if (board_x == client->selected_x && board_y == client->selected_y) {
            // 같은 위치 클릭 - 선택 해제
            client->piece_selected = false;
            add_chat_message_safe("System", "Selection cancelled.");
        } else {
            // 다른 위치 클릭 - 이동 시도
            if (make_move(board, client->selected_x, client->selected_y, board_x, board_y)) {
                add_chat_message_safe("System", "Piece moved.");
                client->piece_selected = false;
                // TODO: 서버에 이동 정보 전송
            } else {
                add_chat_message_safe("System", "Invalid move.");
            }
        }
    }
}

// 마우스 입력 처리
void handle_mouse_input(MEVENT *event) {
    client_state_t *client = get_client_state();

    if (event->bstate & BUTTON1_CLICKED) {
        if (client->current_screen == SCREEN_GAME) {
            int board_x, board_y;
            if (coord_to_board_pos(event->x, event->y, &board_x, &board_y)) {
                // 마우스 좌표 변환 디버그
                char mouse_debug[256];
                snprintf(mouse_debug, sizeof(mouse_debug), "마우스 클릭: 화면(%d,%d) -> 보드(%d,%d)",
                         event->x, event->y, board_x, board_y);
                add_chat_message_safe("System", mouse_debug);

                handle_board_click(board_x, board_y);
            }
        }
    }
}

// 현재 커서 위치 (방향키 조작용)
static int  cursor_x    = 0;
static int  cursor_y    = 0;
static bool cursor_mode = false;

// 커서 모드 활성화/비활성화
void enable_cursor_mode() {
    cursor_mode = true;
    cursor_x    = 0;
    cursor_y    = 0;
}

void disable_cursor_mode() {
    cursor_mode = false;
}

bool is_cursor_mode() {
    return cursor_mode;
}

void get_cursor_position(int *x, int *y) {
    // 블랙 플레이어인 경우 커서 좌표도 변환
    client_state_t *client          = get_client_state();
    bool            is_black_player = (client->game_state.local_team == TEAM_BLACK);

    if (is_black_player) {
        *x = 7 - cursor_x;
        *y = 7 - cursor_y;
    } else {
        *x = cursor_x;
        *y = cursor_y;
    }
}

// 키보드 입력으로 체스판 조작
bool handle_keyboard_board_input(int ch) {
    client_state_t *client = get_client_state();

    if (client->current_screen != SCREEN_GAME) {
        return false;
    }

    switch (ch) {
        case KEY_UP:
            if (!cursor_mode) enable_cursor_mode();
            if (cursor_y > 0) cursor_y--;
            return true;

        case KEY_DOWN:
            if (!cursor_mode) enable_cursor_mode();
            if (cursor_y < BOARD_SIZE - 1) cursor_y++;
            return true;

        case KEY_LEFT:
            if (!cursor_mode) enable_cursor_mode();
            if (cursor_x > 0) cursor_x--;
            return true;

        case KEY_RIGHT:
            if (!cursor_mode) enable_cursor_mode();
            if (cursor_x < BOARD_SIZE - 1) cursor_x++;
            return true;

        case ' ':   // 스페이스바
        case '\n':  // 엔터
        case '\r':
            if (cursor_mode) {
                // 커서 위치를 실제 보드 좌표로 변환
                int actual_x, actual_y;
                get_cursor_position(&actual_x, &actual_y);
                handle_board_click(actual_x, actual_y);
                return true;
            }
            return false;

        case 27:  // ESC
            // 기물이 선택되어 있으면 선택 해제
            if (client->piece_selected) {
                client->piece_selected = false;
                return true;
            }
            // 커서 모드가 활성화되어 있으면 비활성화
            if (cursor_mode) {
                disable_cursor_mode();
                return true;
            }
            // 그 외의 경우 ESC는 아무것도 하지 않음 (메인 화면으로 가지 않음)
            return true;

        default:
            return false;
    }
}

// 체스 표기법 파싱 및 처리
bool handle_chess_notation(const char *notation) {
    client_state_t *client = get_client_state();

    if (client->current_screen != SCREEN_GAME) {
        return false;
    }

    // 간단한 표기법 파싱 (예: e2e4, a1b2)
    if (strlen(notation) == 4) {
        // 출발지 파싱
        int from_x = notation[0] - 'a';
        int from_y = 8 - (notation[1] - '0');

        // 도착지 파싱
        int to_x = notation[2] - 'a';
        int to_y = 8 - (notation[3] - '0');

        // 범위 체크
        if (from_x >= 0 && from_x < 8 && from_y >= 0 && from_y < 8 &&
            to_x >= 0 && to_x < 8 && to_y >= 0 && to_y < 8) {
            board_state_t *board = &client->game_state.board_state;

            // 출발지에 기물이 있는지 확인
            piecestate_t *piece = &board->board[from_y][from_x];
            if (piece->piece && !piece->is_dead) {
                // 현재 플레이어의 기물인지 확인
                bool is_black_player = (client->game_state.local_team == TEAM_BLACK);
                if ((is_black_player && piece->color != BLACK) || (!is_black_player && piece->color != WHITE)) {
                    add_chat_message_safe("System", "You can only move your own pieces!");
                    return false;
                }

                // 현재 턴인지 확인
                if (board->current_turn != client->game_state.local_team) {
                    add_chat_message_safe("System", "It's not your turn!");
                    return false;
                }

                // 이동 시도
                if (make_move(board, from_x, from_y, to_x, to_y)) {
                    char move_msg[64];
                    snprintf(move_msg, sizeof(move_msg), "Move: %s", notation);
                    add_chat_message_safe("System", move_msg);
                    // TODO: 서버에 이동 정보 전송
                    return true;
                } else {
                    add_chat_message_safe("System", "Invalid move notation.");
                }
            } else {
                add_chat_message_safe("System", "No piece at starting position.");
            }
        } else {
            add_chat_message_safe("System", "Invalid notation format.");
        }
    } else {
        add_chat_message_safe("System", "Use format: e2e4 (from-to)");
    }

    return false;
}