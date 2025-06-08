#include <pthread.h>
#include <string.h>

#include "../client_network.h"  // send_move_request 함수를 위해 변경
#include "../client_state.h"
#include "../logger.h"
#include "piece.h"
#include "ui.h"

// 화면 좌표를 보드 좌표로 변환
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y) {
    // NULL 포인터 검사
    if (!board_x || !board_y) {
        LOG_ERROR("board_x or board_y is NULL in coord_to_board_pos");
        return false;
    }

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
        // 보드 그리기 로직과 일치시켜야 함:
        // draw_chess_board에서:
        // - actual_row = is_black_player ? row : (7 - row)
        // - actual_col = is_black_player ? (7 - col) : col
        //
        // 역변환하면:
        // - White 플레이어: actual_col = display_x, actual_row = 7 - display_y
        // - Black 플레이어: actual_col = 7 - display_x, actual_row = display_y

        client_state_t *client = get_client_state();
        if (!client) {
            LOG_ERROR("client is NULL in coord_to_board_pos");
            return false;
        }

        bool is_black_player = (client->game_state.local_team == TEAM_BLACK);

        if (is_black_player) {
            // 블랙 플레이어: actual_col = 7 - col, actual_row = row
            *board_x = 7 - display_x;
            *board_y = display_y;
        } else {
            // 화이트 플레이어: actual_col = col, actual_row = 7 - row
            *board_x = display_x;
            *board_y = 7 - display_y;
        }

        // 최종 결과 값이 올바른 범위인지 재확인
        if (*board_x < 0 || *board_x >= 8 || *board_y < 0 || *board_y >= 8) {
            LOG_ERROR("Calculated board coordinates out of bounds: (%d, %d)", *board_x, *board_y);
            return false;
        }

        return true;
    }

    return false;
}

// 보드 클릭 처리
void handle_board_click(int board_x, int board_y) {
    client_state_t *client = get_client_state();

    // NULL 포인터 검사
    if (!client) {
        LOG_ERROR("client is NULL in handle_board_click");
        return;
    }

    pthread_mutex_lock(&screen_mutex);

    game_t *game = &client->game_state.game;

    if (!client->piece_selected) {
        // 기물 선택 - 경계 검사 후 배열 접근
        piecestate_t *piece = &game->board[board_y][board_x];
        if (piece->piece && !piece->is_dead) {
            // 현재 플레이어의 기물인지 확인
            team_t piece_team = piece->color;

            // 좌표 확인 메시지 (간단하게)
            char coord_msg[64];
            snprintf(coord_msg, sizeof(coord_msg), "Selected: %c%d (board[%d][%d])",
                     'a' + board_x, board_y + 1, board_y, board_x);

            // 뮤텍스 잠금을 해제한 후 채팅 메시지 추가 (데드락 방지)
            pthread_mutex_unlock(&screen_mutex);
            add_chat_message_safe("System", coord_msg);

            // 뮤텍스 다시 잠금
            pthread_mutex_lock(&screen_mutex);

            // 현재 턴인지 확인
            if (game->side_to_move != client->game_state.local_team) {
                char turn_msg[128];
                snprintf(turn_msg, sizeof(turn_msg), "Not your turn! Current: %s",
                         (game->side_to_move == TEAM_WHITE) ? "White" : "Black");
                pthread_mutex_unlock(&screen_mutex);
                add_chat_message_safe("System", turn_msg);
                return;
            }

            // 자신의 기물인지 확인
            if (piece_team != client->game_state.local_team) {
                pthread_mutex_unlock(&screen_mutex);
                add_chat_message_safe("System", "That's not your piece!");
                return;
            }

            // 턴 확인 완료 - 자신의 기물을 선택함
            client->selected_x     = board_x;
            client->selected_y     = board_y;
            client->piece_selected = true;

            pthread_mutex_unlock(&screen_mutex);
            add_chat_message_safe("System", "Piece selected. Click destination.");
            return;
        }
    } else {
        // 기물 이동
        if (board_x == client->selected_x && board_y == client->selected_y) {
            // 같은 위치 클릭 - 선택 해제
            client->piece_selected = false;
            pthread_mutex_unlock(&screen_mutex);
            add_chat_message_safe("System", "Selection cancelled.");
            return;
        } else {
            // 좌표 유효성 재검사
            if (client->selected_x < 0 || client->selected_x >= 8 ||
                client->selected_y < 0 || client->selected_y >= 8) {
                LOG_ERROR("Selected coordinates out of bounds: (%d, %d)", client->selected_x, client->selected_y);
                client->piece_selected = false;
                pthread_mutex_unlock(&screen_mutex);
                add_chat_message_safe("System", "Invalid selection, cancelled.");
                return;
            }

            // 다른 위치 클릭 - 서버로 이동 요청 전송
            char from_coord[3], to_coord[3];

            // 보드 좌표를 체스 표기법으로 변환
            // board_x, board_y는 이미 실제 보드 배열 인덱스이므로 직접 변환
            from_coord[0] = 'a' + client->selected_x;
            from_coord[1] = '1' + client->selected_y;  // 실제 보드 좌표를 rank로 변환
            from_coord[2] = '\0';

            to_coord[0] = 'a' + board_x;
            to_coord[1] = '1' + board_y;  // 실제 보드 좌표를 rank로 변환
            to_coord[2] = '\0';

            // 뮤텍스 잠금을 해제한 후 네트워크 함수 호출 (데드락 방지)
            pthread_mutex_unlock(&screen_mutex);

            // 서버로 이동 요청 전송
            if (send_move_request(from_coord, to_coord) == 0) {
                char move_msg[64];
                snprintf(move_msg, sizeof(move_msg), "Move request sent: %s -> %s", from_coord, to_coord);
                add_chat_message_safe("System", move_msg);

                // 뮤텍스 다시 잠금하여 상태 변경
                pthread_mutex_lock(&screen_mutex);
                client->piece_selected = false;
                pthread_mutex_unlock(&screen_mutex);
            } else {
                add_chat_message_safe("System", "Failed to send move request.");
            }
            return;
        }
    }

    // 함수 끝에서 뮤텍스 해제
    pthread_mutex_unlock(&screen_mutex);
}

// 마우스 입력 처리
void handle_mouse_input(MEVENT *event) {
    // NULL 포인터 검사
    if (!event) {
        LOG_ERROR("event is NULL in handle_mouse_input");
        return;
    }

    client_state_t *client = get_client_state();
    if (!client) {
        LOG_ERROR("client is NULL in handle_mouse_input");
        return;
    }

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
    // NULL 포인터 검사
    if (!x || !y) {
        LOG_ERROR("x or y is NULL in get_cursor_position");
        return;
    }

    // 내부 커서 좌표를 그대로 반환 (방향키 처리에서 이미 플레이어 관점을 고려함)
    *x = cursor_x;
    *y = cursor_y;

    // 최종 결과가 유효한 범위인지 확인
    if (*x < 0 || *x >= 8 || *y < 0 || *y >= 8) {
        LOG_ERROR("Cursor position out of bounds: (%d, %d)", *x, *y);
        *x = 0;
        *y = 0;
    }
}

// 키보드 입력으로 체스판 조작
bool handle_keyboard_board_input(int ch) {
    client_state_t *client = get_client_state();

    // NULL 포인터 검사
    if (!client) {
        LOG_ERROR("client is NULL in handle_keyboard_board_input");
        return false;
    }

    if (client->current_screen != SCREEN_GAME) {
        return false;
    }

    switch (ch) {
        case KEY_UP:
            if (!cursor_mode) enable_cursor_mode();
            // 화면상 위쪽으로 이동 (row 감소)
            if (cursor_y > 0) cursor_y--;
            return true;

        case KEY_DOWN:
            if (!cursor_mode) enable_cursor_mode();
            // 화면상 아래쪽으로 이동 (row 증가)
            if (cursor_y < BOARD_SIZE - 1) cursor_y++;
            return true;

        case KEY_LEFT:
            if (!cursor_mode) enable_cursor_mode();
            // 화면상 왼쪽으로 이동 (col 감소)
            if (cursor_x > 0) cursor_x--;
            return true;

        case KEY_RIGHT:
            if (!cursor_mode) enable_cursor_mode();
            // 화면상 오른쪽으로 이동 (col 증가)
            if (cursor_x < BOARD_SIZE - 1) cursor_x++;
            return true;

        case ' ':   // 스페이스바
        case '\n':  // 엔터
        case '\r':
            if (cursor_mode) {
                // 커서 위치를 실제 보드 좌표로 변환해서 처리
                client_state_t *client          = get_client_state();
                bool            is_black_player = (client->game_state.local_team == TEAM_BLACK);

                int actual_x, actual_y;
                if (is_black_player) {
                    actual_x = 7 - cursor_x;  // 블랙 플레이어: 좌우 반전
                    actual_y = cursor_y;      // 블랙 플레이어: 상하 그대로
                } else {
                    actual_x = cursor_x;      // 화이트 플레이어: 좌우 그대로
                    actual_y = 7 - cursor_y;  // 화이트 플레이어: 상하 반전
                }

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

    // NULL 포인터 검사
    if (!notation) {
        LOG_ERROR("notation is NULL in handle_chess_notation");
        return false;
    }

    if (client->current_screen != SCREEN_GAME) {
        return false;
    }

    if (strlen(notation) == 4) {
        // e2e4 형식
        if (client->current_screen == SCREEN_GAME) {
            // 체스 표기법을 좌표로 변환
            int from_x = notation[0] - 'a';
            int from_y = notation[1] - '1';
            int to_x   = notation[2] - 'a';
            int to_y   = notation[3] - '1';

            // 범위 검사
            if (from_x < 0 || from_x >= 8 || from_y < 0 || from_y >= 8 ||
                to_x < 0 || to_x >= 8 || to_y < 0 || to_y >= 8) {
                add_chat_message_safe("System", "Invalid coordinates");
                return false;
            }

            game_t *game = &client->game_state.game;

            // 기물 확인
            piecestate_t *piece = &game->board[from_y][from_x];

            // 기물이 있는지 확인
            if (!piece->piece || piece->is_dead) {
                add_chat_message_safe("System", "No piece at source position");
                return false;
            }

            // 현재 턴인지 확인
            if (game->side_to_move != client->game_state.local_team) {
                add_chat_message_safe("System", "Not your turn!");
                return false;
            }

            // 서버로 이동 요청 전송
            char from_coord[3], to_coord[3];
            from_coord[0] = notation[0];
            from_coord[1] = notation[1];
            from_coord[2] = '\0';
            to_coord[0]   = notation[2];
            to_coord[1]   = notation[3];
            to_coord[2]   = '\0';

            if (send_move_request(from_coord, to_coord) == 0) {
                char move_msg[64];
                snprintf(move_msg, sizeof(move_msg), "Move request sent: %s", notation);
                add_chat_message_safe("System", move_msg);
                return true;
            } else {
                add_chat_message_safe("System", "Failed to send move request.");
            }
        }
    } else {
        add_chat_message_safe("System", "Use format: e2e4 (from-to)");
    }

    return false;
}