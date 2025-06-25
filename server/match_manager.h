#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "message.pb-c.h"
#include "rule.h"  // 체스 게임 상태 관리를 위해 추가

#define MAX_WAITING_PLAYERS 100
#define MAX_ACTIVE_GAMES    50
#define GAME_ID_LENGTH      32

// 매칭 상태 열거형
typedef enum {
    MATCH_STATUS_WAITING,       // 상대방 대기 중
    MATCH_STATUS_GAME_STARTED,  // 게임 시작됨
    MATCH_STATUS_ERROR          // 오류 발생
} MatchStatus;

// 대기 중인 플레이어 정보
typedef struct
{
    int    fd;               // 클라이언트 소켓 파일 디스크립터
    char   player_id[64];    // 플레이어 ID
    time_t wait_start_time;  // 대기 시작 시간
    bool   is_active;        // 활성 상태
} WaitingPlayer;

// 활성 게임 정보
typedef struct
{
    char   game_id[GAME_ID_LENGTH + 1];  // 게임 ID
    int    white_player_fd;              // 흰색 플레이어 소켓
    int    black_player_fd;              // 검은색 플레이어 소켓
    char   white_player_id[64];          // 흰색 플레이어 ID
    char   black_player_id[64];          // 검은색 플레이어 ID
    time_t game_start_time;              // 게임 시작 시간
    bool   is_active;                    // 게임 활성 상태
    game_t game_state;                   // 체스 게임 보드 상태

    // 타이머 관련 정보
    int32_t time_limit_per_player;  // 각 플레이어별 제한시간 (초)
    int32_t white_time_remaining;   // 백 팀 남은 시간 (초)
    int32_t black_time_remaining;   // 흑 팀 남은 시간 (초)
    time_t  last_move_time;         // 마지막 이동 시간 (턴 시간 계산용)
} ActiveGame;

// 매칭 결과 구조체
typedef struct
{
    MatchStatus status;         // 매칭 상태
    char       *game_id;        // 게임 ID (성공 시)
    Team        assigned_team;  // 할당된 색상
    int         opponent_fd;    // 상대방 소켓 (게임 시작 시)
    char       *opponent_name;  // 상대방 이름 (게임 시작 시)
    char       *error_message;  // 오류 메시지 (실패 시)
} MatchResult;

// 매칭 매니저 구조체
typedef struct
{
    WaitingPlayer   waiting_players[MAX_WAITING_PLAYERS];
    ActiveGame      active_games[MAX_ACTIVE_GAMES];
    int             waiting_count;
    int             active_game_count;
    pthread_mutex_t mutex;  // 스레드 안전성을 위한 뮤텍스
} MatchManager;

// 매칭 매니저 전역 변수
extern MatchManager g_match_manager;

// 함수 선언
int         init_match_manager(void);
void        cleanup_match_manager(void);
MatchResult add_player_to_matching(int fd, const char *player_id);
int         remove_player_from_matching(int fd);
ActiveGame *find_game_by_player_fd(int fd);
int         remove_game(const char *game_id);
char       *generate_game_id(void);
int         handle_player_disconnect(int fd);

// 디버깅/모니터링 함수
void print_match_manager_status(void);
int  get_waiting_players_count(void);
int  get_active_games_count(void);

#endif  // MATCH_MANAGER_H