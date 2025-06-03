#include "ui.h"
#include "client_state.h"

// 화면 좌표를 보드 좌표로 변환
bool coord_to_board_pos(int screen_x, int screen_y, int *board_x, int *board_y)
{
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
    int total_chess_width = SQUARE_W * BOARD_SIZE;
    int total_chess_height = SQUARE_H * BOARD_SIZE;

    if (chess_rel_x >= total_chess_width || chess_rel_y >= total_chess_height)
        return false;

    // 어느 칸인지 계산
    *board_x = chess_rel_x / SQUARE_W;
    *board_y = chess_rel_y / SQUARE_H;

    // 범위 확인
    if (*board_x >= 0 && *board_x < BOARD_SIZE && *board_y >= 0 && *board_y < BOARD_SIZE)
    {
        return true;
    }

    return false;
}

// 보드 클릭 처리
void handle_board_click(int board_x, int board_y)
{
    client_state_t *client = get_client_state();

    if (client->current_screen != SCREEN_GAME)
        return;

    board_state_t *board = &client->game_state.board_state;

    if (!client->piece_selected)
    {
        // 기물 선택
        piecestate_t *piece = &board->board[board_y][board_x];
        if (piece->piece && !piece->is_dead)
        {
            // TODO: 현재 플레이어의 기물인지 확인
            client->selected_x = board_x;
            client->selected_y = board_y;
            client->piece_selected = true;

            add_chat_message_safe("System", "Piece selected. Click destination.");
        }
    }
    else
    {
        // 기물 이동
        if (board_x == client->selected_x && board_y == client->selected_y)
        {
            // 같은 위치 클릭 - 선택 해제
            client->piece_selected = false;
            add_chat_message_safe("System", "Selection cancelled.");
        }
        else
        {
            // 다른 위치 클릭 - 이동 시도
            if (make_move(board, client->selected_x, client->selected_y, board_x, board_y))
            {
                add_chat_message_safe("System", "Piece moved.");
                client->piece_selected = false;
                // TODO: 서버에 이동 정보 전송
            }
            else
            {
                add_chat_message_safe("System", "Invalid move.");
            }
        }
    }
}

// 마우스 입력 처리
void handle_mouse_input(MEVENT *event)
{
    client_state_t *client = get_client_state();

    if (event->bstate & BUTTON1_CLICKED)
    {
        if (client->current_screen == SCREEN_GAME)
        {
            int board_x, board_y;
            if (coord_to_board_pos(event->x, event->y, &board_x, &board_y))
            {
                handle_board_click(board_x, board_y);
            }
        }
    }
}