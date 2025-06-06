#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <time.h>

#include "piece.h"

#define BOARD_SIZE         8
#define MAX_CHAT_MESSAGES  50
#define MAX_MESSAGE_LENGTH 256

// 팀 정의
typedef enum {
    TEAM_WHITE = 0,
    TEAM_BLACK = 1
} team_t;

// 채팅 메시지 구조체
typedef struct
{
    char   sender[32];
    char   message[MAX_MESSAGE_LENGTH];
    time_t timestamp;
} chat_message_t;

// 게임 보드 상태
typedef struct
{
    piecestate_t board[BOARD_SIZE][BOARD_SIZE];
    bool         white_king_moved;
    bool         black_king_moved;
    bool         white_rook_kingside_moved;
    bool         white_rook_queenside_moved;
    bool         black_rook_kingside_moved;
    bool         black_rook_queenside_moved;
    int          en_passant_target_x;
    int          en_passant_target_y;
    team_t       current_turn;
    bool         is_check;
    bool         is_checkmate;
    bool         is_stalemate;
    int          halfmove_clock;
    int          fullmove_number;
} board_state_t;

// 게임 상태
typedef struct
{
    board_state_t  board_state;
    chat_message_t chat_messages[MAX_CHAT_MESSAGES];
    int            chat_count;
    char           local_player[32];
    char           opponent_player[32];
    team_t         local_team;
    bool           game_in_progress;
    time_t         game_start_time;
    int            white_time_remaining;  // 초 단위
    int            black_time_remaining;  // 초 단위

    // 상대방 연결 끊김 감지
    bool opponent_disconnected;
    char opponent_disconnect_message[256];
} game_state_t;

// 함수 선언
void init_game_state(game_state_t *state);
void init_board_state(board_state_t *board);
void reset_board_to_starting_position(board_state_t *board);
void add_chat_message(game_state_t *state, const char *sender, const char *message);
bool is_valid_move(board_state_t *board, int from_x, int from_y, int to_x, int to_y);
bool make_move(board_state_t *board, int from_x, int from_y, int to_x, int to_y);
bool is_in_check(board_state_t *board, team_t team);
bool is_checkmate(board_state_t *board, team_t team);
bool is_stalemate(board_state_t *board, team_t team);

// 편의 함수들 (클라이언트 상태 호환성)
bool        game_is_white_player(const game_state_t *state);
const char *get_opponent_name(const game_state_t *state);

#endif  // GAME_STATE_H