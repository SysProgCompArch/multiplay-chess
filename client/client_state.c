#include "client_state.h"

#include <string.h>

#include "client_network.h"
#include "logger.h"

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
    g_client.current_screen   = SCREEN_MAIN;
    g_client.connected        = false;
    g_client.piece_selected   = false;
    g_client.socket_fd        = -1;
    g_client.match_start_time = 0;
    g_client.selected_x       = -1;
    g_client.selected_y       = -1;

    // 채팅 입력 관련 초기화
    g_client.chat_input_mode   = false;
    g_client.chat_input_cursor = 0;
    memset(g_client.chat_input_buffer, 0, sizeof(g_client.chat_input_buffer));

    // 에러 다이얼로그 상태 초기화
    g_client.dialog_active = false;

    // 기권 확인 다이얼로그 상태 초기화
    g_client.resign_dialog_active = false;

    // 기권 결과 다이얼로그 상태 초기화
    g_client.resign_result_dialog_pending = false;
    memset(g_client.resign_result_title, 0, sizeof(g_client.resign_result_title));
    memset(g_client.resign_result_message, 0, sizeof(g_client.resign_result_message));

    // 게임 종료 다이얼로그 상태 초기화
    g_client.game_end_dialog_pending = false;
    memset(g_client.game_end_message, 0, sizeof(g_client.game_end_message));

    // 연결 끊김 감지 초기화
    g_client.connection_lost = false;
    memset(g_client.disconnect_message, 0, sizeof(g_client.disconnect_message));

    // 서버 연결 정보 기본값 설정
    strcpy(g_client.server_host, SERVER_DEFAULT_HOST);
    g_client.server_port = SERVER_DEFAULT_PORT;

    // 화면 업데이트 플래그 초기화
    g_client.screen_update_requested = false;

    init_game_state(&g_client.game_state);
}

// 채팅 메시지 추가 (게임 상태에 추가)
void add_chat_message_safe(const char *sender, const char *message) {
    pthread_mutex_lock(&screen_mutex);
    add_chat_message(&g_client.game_state, sender, message);
    pthread_mutex_unlock(&screen_mutex);
}

// 편의 함수들 (게임 상태에서 가져오기)
bool client_is_white() {
    return game_is_white_player(&g_client.game_state);
}

const char *get_opponent_name_client() {
    return get_opponent_name(&g_client.game_state);
}