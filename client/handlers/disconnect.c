#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

// 상대방 연결 끊김 브로드캐스트 처리
int handle_opponent_disconnected_broadcast(ServerMessage *msg) {
    if (!msg || !msg->opponent_disconnected) {
        LOG_ERROR("Invalid opponent disconnected broadcast message");
        return -1;
    }

    OpponentDisconnectedBroadcast *disconnect_msg = msg->opponent_disconnected;
    LOG_INFO("Opponent disconnected: player_id=%s, game_id=%s, winner_color=%d",
             disconnect_msg->player_id ? disconnect_msg->player_id : "unknown",
             disconnect_msg->game_id ? disconnect_msg->game_id : "unknown",
             disconnect_msg->winner_color);

    // 채팅 메시지로 알림
    char        notification[256];
    const char *winner_text = (disconnect_msg->winner_color == COLOR__COLOR_WHITE) ? "White" : "Black";
    snprintf(notification, sizeof(notification),
             "Opponent disconnected. %s team wins!", winner_text);
    add_chat_message_safe("System", notification);

    // 상대방 연결 끊김 다이얼로그 설정
    client_state_t *client = get_client_state();
    pthread_mutex_lock(&screen_mutex);
    client->game_state.opponent_disconnected = true;
    snprintf(client->game_state.opponent_disconnect_message, sizeof(client->game_state.opponent_disconnect_message),
             "Your opponent has disconnected.\n%s team wins the game!", winner_text);

    // 화면 업데이트 요청
    client->screen_update_requested = true;
    pthread_mutex_unlock(&screen_mutex);

    // 게임 상태 초기화
    // 필요한 경우 게임 상태 정리
    LOG_DEBUG("Game state cleanup after opponent disconnect");

    LOG_INFO("Returned to main screen after opponent disconnect");
    return 0;
}