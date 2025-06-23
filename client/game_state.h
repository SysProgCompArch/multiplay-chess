#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <time.h>

#include "piece.h"
#include "rule.h"  // game_t 사용을 위해 추가
#include "types.h"
#include "pgn.h"

#define BOARD_SIZE         8
#define MAX_CHAT_MESSAGES  50
#define MAX_MESSAGE_LENGTH 256

// 채팅 메시지 구조체
typedef struct
{
    char   sender[MAX_PLAYER_NAME_LENGTH];
    char   message[MAX_MESSAGE_LENGTH];
    time_t timestamp;
} chat_message_t;

// 게임 상태 (game_t를 직접 사용)
typedef struct
{
    game_t         game;  // 통합된 게임 상태
    chat_message_t chat_messages[MAX_CHAT_MESSAGES];
    int            chat_count;
    char           local_player[MAX_PLAYER_NAME_LENGTH];
    char           opponent_player[MAX_PLAYER_NAME_LENGTH];
    team_t         local_team;
    bool           game_in_progress;
    time_t         game_start_time;
    int            white_time_remaining;  // 초 단위
    int            black_time_remaining;  // 초 단위
    

    // 이동 가능 위치 표시 (UI용)
    bool possible_moves[BOARD_SIZE][BOARD_SIZE];  // 이동 가능한 위치 표시
    bool capture_moves[BOARD_SIZE][BOARD_SIZE];   // 캡처 가능한 위치 표시

    // 상대방 연결 끊김 감지
    bool opponent_disconnected;
    char opponent_disconnect_message[256];

    // 체크 상태 정보
    bool white_in_check;
    bool black_in_check;
} game_state_t;

// 함수 선언
void init_game_state(game_state_t *state);
void init_game(game_t *game);                                  // board_state_t 대신 game_t 초기화
void reset_game_for_player(game_t *game, team_t player_team);  // 플레이어 팀에 따른 보드 초기화
void reset_game_to_starting_position(game_t *game);
void add_chat_message(game_state_t *state, const char *sender, const char *message);\

// 이동 관련 (game_t 직접 사용)
bool is_valid_move(game_t *game, int from_x, int from_y, int to_x, int to_y);
bool make_move(game_t *game, int from_x, int from_y, int to_x, int to_y);

// 편의 함수들 (클라이언트 상태 호환성)
bool        game_is_white_player(const game_state_t *state);
const char *get_opponent_name(const game_state_t *state);

// 서버 동기화 함수
bool apply_move_from_server(game_t *game, const char *from, const char *to);

#endif  // GAME_STATE_H