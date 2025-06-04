#include "client_state.h"

#include <string.h>

// 전역 클라이언트 상태
static client_state_t g_client = {0};

// 스레드 동기화 변수들
pthread_mutex_t screen_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t network_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t       network_thread_id;
bool            network_thread_running = false;

// 클라이언트 상태 접근
client_state_t *get_client_state() {
    return &g_client;
}

// 클라이언트 상태 초기화
void init_client_state() {
    memset(&g_client, 0, sizeof(g_client));
    g_client.current_screen = SCREEN_MAIN;
    g_client.connected      = false;
    g_client.piece_selected = false;
    g_client.socket_fd      = -1;
    init_game_state(&g_client.game_state);
}

// 안전한 채팅 메시지 추가
void add_chat_message_safe(const char *sender, const char *message) {
    pthread_mutex_lock(&screen_mutex);
    add_chat_message(&g_client.game_state, sender, message);
    pthread_mutex_unlock(&screen_mutex);
}