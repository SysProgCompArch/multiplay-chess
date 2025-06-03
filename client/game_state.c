#include "game_state.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// piece_table 외부 선언 (common/piece.c에 정의됨)
extern piece_t *piece_table[2][6];

// 게임 상태 초기화
void init_game_state(game_state_t *state)
{
    memset(state, 0, sizeof(game_state_t));
    init_board_state(&state->board_state);
    state->chat_count = 0;
    state->game_in_progress = false;
    state->white_time_remaining = 600; // 10분
    state->black_time_remaining = 600; // 10분
}

// 보드 상태 초기화
void init_board_state(board_state_t *board)
{
    memset(board, 0, sizeof(board_state_t));

    // 보드 초기화 (모든 칸을 빈 칸으로)
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            board->board[i][j].piece = NULL;
            board->board[i][j].x = j;
            board->board[i][j].y = i;
            board->board[i][j].is_dead = false;
            board->board[i][j].is_promoted = false;
        }
    }

    board->en_passant_target_x = -1;
    board->en_passant_target_y = -1;
    board->current_turn = TEAM_WHITE;
    board->halfmove_clock = 0;
    board->fullmove_number = 1;

    reset_board_to_starting_position(board);
}

// 체스 보드를 시작 위치로 설정
void reset_board_to_starting_position(board_state_t *board)
{
    // 모든 칸 비우기
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board->board[i][j].piece = NULL;

    // 흰색 기물 (아래쪽) - piece_table[0][piece_type] 사용
    for (int x = 0; x < 8; x++)
    {
        board->board[6][x].piece = piece_table[0][PAWN]; // 흰색 폰
        board->board[6][x].is_dead = false;
        board->board[6][x].x = x;
        board->board[6][x].y = 6;
        board->board[6][x].color = 0; // WHITE
        board->board[6][x].is_first_move = true;
        board->board[6][x].is_promoted = false;
    }
    board->board[7][0].piece = piece_table[0][ROOK];   // 흰색 룩
    board->board[7][1].piece = piece_table[0][KNIGHT]; // 흰색 나이트
    board->board[7][2].piece = piece_table[0][BISHOP]; // 흰색 비숍
    board->board[7][3].piece = piece_table[0][QUEEN];  // 흰색 퀸
    board->board[7][4].piece = piece_table[0][KING];   // 흰색 킹
    board->board[7][5].piece = piece_table[0][BISHOP]; // 흰색 비숍
    board->board[7][6].piece = piece_table[0][KNIGHT]; // 흰색 나이트
    board->board[7][7].piece = piece_table[0][ROOK];   // 흰색 룩
    for (int x = 0; x < 8; x++)
    {
        board->board[7][x].is_dead = false;
        board->board[7][x].x = x;
        board->board[7][x].y = 7;
        board->board[7][x].color = 0; // WHITE
        board->board[7][x].is_first_move = true;
        board->board[7][x].is_promoted = false;
    }

    // 검은색 기물 (위쪽) - piece_table[1][piece_type] 사용
    for (int x = 0; x < 8; x++)
    {
        board->board[1][x].piece = piece_table[1][PAWN]; // 검은색 폰
        board->board[1][x].is_dead = false;
        board->board[1][x].x = x;
        board->board[1][x].y = 1;
        board->board[1][x].color = 1; // BLACK
        board->board[1][x].is_first_move = true;
        board->board[1][x].is_promoted = false;
    }
    board->board[0][0].piece = piece_table[1][ROOK];   // 검은색 룩
    board->board[0][1].piece = piece_table[1][KNIGHT]; // 검은색 나이트
    board->board[0][2].piece = piece_table[1][BISHOP]; // 검은색 비숍
    board->board[0][3].piece = piece_table[1][QUEEN];  // 검은색 퀸
    board->board[0][4].piece = piece_table[1][KING];   // 검은색 킹
    board->board[0][5].piece = piece_table[1][BISHOP]; // 검은색 비숍
    board->board[0][6].piece = piece_table[1][KNIGHT]; // 검은색 나이트
    board->board[0][7].piece = piece_table[1][ROOK];   // 검은색 룩
    for (int x = 0; x < 8; x++)
    {
        board->board[0][x].is_dead = false;
        board->board[0][x].x = x;
        board->board[0][x].y = 0;
        board->board[0][x].color = 1; // BLACK
        board->board[0][x].is_first_move = true;
        board->board[0][x].is_promoted = false;
    }
}

// 채팅 메시지 추가
void add_chat_message(game_state_t *state, const char *sender, const char *message)
{
    if (state->chat_count >= MAX_CHAT_MESSAGES)
    {
        // 배열이 가득 찬 경우 가장 오래된 메시지 제거
        for (int i = 1; i < MAX_CHAT_MESSAGES; i++)
        {
            state->chat_messages[i - 1] = state->chat_messages[i];
        }
        state->chat_count = MAX_CHAT_MESSAGES - 1;
    }

    chat_message_t *msg = &state->chat_messages[state->chat_count];
    strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    msg->sender[sizeof(msg->sender) - 1] = '\0';

    strncpy(msg->message, message, sizeof(msg->message) - 1);
    msg->message[sizeof(msg->message) - 1] = '\0';

    msg->timestamp = time(NULL);
    state->chat_count++;
}

// 유효한 수인지 확인 (기본 체크)
bool is_valid_move(board_state_t *board, int from_x, int from_y, int to_x, int to_y)
{
    // 보드 경계 체크
    if (from_x < 0 || from_x >= BOARD_SIZE || from_y < 0 || from_y >= BOARD_SIZE ||
        to_x < 0 || to_x >= BOARD_SIZE || to_y < 0 || to_y >= BOARD_SIZE)
    {
        return false;
    }

    // 출발 위치에 기물이 있는지 확인
    piecestate_t *piece = &board->board[from_y][from_x];
    if (!piece->piece || piece->is_dead)
    {
        return false;
    }

    // 같은 위치로 이동하는지 확인
    if (from_x == to_x && from_y == to_y)
    {
        return false;
    }

    // TODO: 실제 체스 규칙 검증은 common/rule.c 함수들을 사용

    return true;
}

// 실제 수 실행
bool make_move(board_state_t *board, int from_x, int from_y, int to_x, int to_y)
{
    if (!is_valid_move(board, from_x, from_y, to_x, to_y))
    {
        return false;
    }

    // 기물 이동
    piecestate_t *from_piece = &board->board[from_y][from_x];
    piecestate_t *to_piece = &board->board[to_y][to_x];

    // 목표 위치에 기물이 있다면 제거
    if (to_piece->piece && !to_piece->is_dead)
    {
        to_piece->is_dead = true;
    }

    // 기물 이동
    *to_piece = *from_piece;
    to_piece->x = to_x;
    to_piece->y = to_y;

    // 원래 위치 비우기
    from_piece->piece = NULL;
    from_piece->is_dead = false;

    // 턴 변경
    board->current_turn = (board->current_turn == TEAM_WHITE) ? TEAM_BLACK : TEAM_WHITE;

    // TODO: 특수 규칙 처리 (캐슬링, 앙파상, 프로모션 등)

    return true;
}

// 체크 상태 확인
bool is_in_check(board_state_t *board, team_t team)
{
    // TODO: 실제 체크 검증 로직 구현
    return false;
}

// 체크메이트 확인
bool is_checkmate(board_state_t *board, team_t team)
{
    // TODO: 실제 체크메이트 검증 로직 구현
    return false;
}

// 스테일메이트 확인
bool is_stalemate(board_state_t *board, team_t team)
{
    // TODO: 실제 스테일메이트 검증 로직 구현
    return false;
}