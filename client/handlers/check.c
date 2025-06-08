#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

// 체크 브로드캐스트 처리
int handle_check_broadcast(ServerMessage *msg) {
    if (!msg || msg->msg_case != SERVER_MESSAGE__MSG_CHECK_BROADCAST) {
        LOG_ERROR("Invalid check broadcast message");
        return -1;
    }

    CheckBroadcast *check_broadcast = msg->check_broadcast;
    if (!check_broadcast) {
        LOG_ERROR("Check broadcast is null");
        return -1;
    }

    LOG_INFO("Received check broadcast: player_id=%s, by_color=%d, game_id=%s",
             check_broadcast->player_id ? check_broadcast->player_id : "unknown",
             check_broadcast->by_color,
             check_broadcast->game_id ? check_broadcast->game_id : "unknown");

    // 체크 상태를 게임 상태에 반영
    client_state_t *client = get_client_state();

    pthread_mutex_lock(&screen_mutex);

    // 게임 상태에 체크 정보 저장
    if (check_broadcast->by_color == COLOR__COLOR_WHITE) {
        client->game_state.white_in_check = true;
    } else if (check_broadcast->by_color == COLOR__COLOR_BLACK) {
        client->game_state.black_in_check = true;
    }

    // 화면 업데이트 요청
    client->screen_update_requested = true;

    pthread_mutex_unlock(&screen_mutex);

    // 채팅 메시지로 체크 알림
    char        check_msg[256];
    const char *checked_player = (check_broadcast->by_color == COLOR__COLOR_WHITE) ? "White" : "Black";
    snprintf(check_msg, sizeof(check_msg), "%s is in check!", checked_player);
    add_chat_message_safe("Game", check_msg);

    LOG_INFO("Check state updated for %s player", checked_player);
    return 0;
}