#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "common.h"
#include "game_state.h"

// 서버 연결 설정
#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8080

// 상수 정의
#define MAX_CHAT_NAME   32
#define MAX_PLAYER_NAME 32

// 화면 상태 enum
typedef enum {
    SCREEN_MAIN,
    SCREEN_MATCHING,
    SCREEN_GAME
} screen_state_t;

// 클라이언트 상태 정보
typedef struct
{
    char           username[MAX_CHAT_NAME];
    int            socket_fd;
    bool           connected;
    screen_state_t current_screen;
    time_t         match_start_time;
    game_state_t   game_state;
    int            selected_x, selected_y;  // 선택된 기물 위치
    bool           piece_selected;          // 기물이 선택되었는지 여부

    // 채팅 입력 모드
    bool chat_input_mode;         // 채팅 입력 모드 활성화 여부
    char chat_input_buffer[256];  // 채팅 입력 버퍼
    int  chat_input_cursor;       // 채팅 입력 커서 위치

    // 재연결 관련
    bool   reconnecting;
    time_t last_reconnect_attempt;

    // 에러 다이얼로그 상태
    bool dialog_active;

    // 연결 끊김 감지
    bool connection_lost;
    char disconnect_message[256];
} client_state_t;

// 전역 상태 접근 함수들
client_state_t *get_client_state();
void            init_client_state();

// 스레드 동기화
extern pthread_mutex_t screen_mutex;
extern pthread_mutex_t network_mutex;
extern pthread_t       network_thread_id;
extern bool            network_thread_running;

// 채팅 메시지 추가 (게임 상태에 추가)
void add_chat_message_safe(const char *sender, const char *message);

// 편의 함수들 (게임 상태에서 가져오기)
bool        client_is_white();
const char *get_opponent_name_client();

#endif  // CLIENT_STATE_H