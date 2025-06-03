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

// 화면 상태 enum
typedef enum
{
    SCREEN_MAIN,
    SCREEN_MATCHING,
    SCREEN_GAME
} screen_state_t;

// 클라이언트 상태 정보
typedef struct
{
    char username[32];
    char opponent_name[32];
    int socket_fd;
    bool connected;
    bool is_white;
    screen_state_t current_screen;
    time_t match_start_time;
    game_state_t game_state;
    int selected_x, selected_y; // 선택된 기물 위치
    bool piece_selected;        // 기물이 선택되었는지 여부
} client_state_t;

// 전역 상태 접근 함수들
client_state_t *get_client_state();
void init_client_state();

// 스레드 동기화
extern pthread_mutex_t screen_mutex;
extern pthread_mutex_t network_mutex;
extern pthread_t network_thread_id;
extern bool network_thread_running;

// 안전한 채팅 메시지 추가 (mutex 처리됨)
void add_chat_message_safe(const char *sender, const char *message);

#endif // CLIENT_STATE_H