#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

// 기권 응답 처리
int handle_resign_response(ServerMessage *msg) {
    if (!msg || !msg->resign_res) {
        LOG_ERROR("Invalid resign response message");
        return -1;
    }

    ResignResponse *resign_res = msg->resign_res;
    LOG_INFO("Received resign response: success=%s, message=%s",
             resign_res->success ? "true" : "false",
             resign_res->message ? resign_res->message : "");

    if (resign_res->success) {
        add_chat_message_safe("System", "Resignation successful. Game will end shortly.");
        LOG_INFO("Resignation processed successfully");
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Resignation failed: %s",
                 resign_res->message ? resign_res->message : "Unknown error");
        add_chat_message_safe("System", error_msg);
        LOG_WARN("Resignation failed: %s", resign_res->message ? resign_res->message : "Unknown error");
    }

    return 0;
}

// 기권 브로드캐스트 처리
int handle_resign_broadcast(ServerMessage *msg) {
    if (!msg || !msg->resign_broadcast) {
        LOG_ERROR("Invalid resign broadcast message");
        return -1;
    }

    ResignBroadcast *resign_broadcast = msg->resign_broadcast;
    LOG_INFO("Received resign broadcast: player_id=%s, game_id=%s, winner_team=%d",
             resign_broadcast->player_id ? resign_broadcast->player_id : "unknown",
             resign_broadcast->game_id ? resign_broadcast->game_id : "unknown",
             resign_broadcast->winner_team);

    // 게임 종료 다이얼로그 설정
    client_state_t *client = get_client_state();

    // 내가 기권한 경우와 상대가 기권한 경우 구분
    const char *resigning_player = resign_broadcast->player_id ? resign_broadcast->player_id : "Unknown";
    bool        i_resigned       = (strcmp(resigning_player, client->username) == 0);

    // 뮤텍스 잠그기 전에 채팅 메시지 추가 (데드락 방지)
    if (i_resigned) {
        add_chat_message_safe("System", "You resigned the game.");
    } else {
        add_chat_message_safe("System", "Opponent resigned the game. You win!");
    }

    pthread_mutex_lock(&screen_mutex);

    // 기권 전용 다이얼로그 설정
    client->resign_result_dialog_pending = true;

    if (i_resigned) {
        strcpy(client->resign_result_title, "Game Over - Resignation");
        strcpy(client->resign_result_message, "You resigned the game.\nYou lose by resignation!");
    } else {
        strcpy(client->resign_result_title, "Game Over - Victory");
        strcpy(client->resign_result_message, "Opponent resigned the game.\nYou win by resignation!");
    }

    // 화면 업데이트 요청
    client->screen_update_requested = true;
    pthread_mutex_unlock(&screen_mutex);

    // 게임 상태 초기화
    LOG_DEBUG("Game state cleanup after resignation");

    LOG_INFO("Returned to main screen after resignation");
    return 0;
}