#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../client_state.h"
#include "handlers.h"
#include "logger.h"

// 게임 종료 브로드캐스트 처리
int handle_game_end_broadcast(ServerMessage *msg) {
    if (!msg || !msg->game_end) {
        LOG_ERROR("Invalid game end broadcast message");
        return -1;
    }

    GameEndBroadcast *game_end_msg = msg->game_end;
    LOG_INFO("Game ended: player_id=%s, game_id=%s, winner_team=%d, end_type=%d",
             game_end_msg->player_id ? game_end_msg->player_id : "unknown",
             game_end_msg->game_id ? game_end_msg->game_id : "unknown",
             game_end_msg->winner_team, game_end_msg->end_type);

    // 종료 유형에 따른 메시지 생성
    char        notification[256];
    char        dialog_message[512];
    const char *winner_text = (game_end_msg->winner_team == TEAM__TEAM_WHITE) ? "White" : "Black";
    const char *end_reason;

    switch (game_end_msg->end_type) {
        case GAME_END_TYPE__GAME_END_DISCONNECT:
            end_reason = "opponent disconnected";
            break;
        case GAME_END_TYPE__GAME_END_RESIGN:
            end_reason = "resignation";
            break;
        case GAME_END_TYPE__GAME_END_CHECKMATE:
            end_reason = "checkmate";
            break;
        case GAME_END_TYPE__GAME_END_STALEMATE:
            end_reason = "stalemate";
            break;
        case GAME_END_TYPE__GAME_END_DRAW:
            end_reason = "draw";
            break;
        case GAME_END_TYPE__GAME_END_TIMEOUT:
            end_reason = "timeout";
            break;
        default:
            end_reason = "unknown reason";
            break;
    }

    // 게임 종료 다이얼로그 설정
    client_state_t *client = get_client_state();
    pthread_mutex_lock(&screen_mutex);

    // 기권은 별도의 ResignBroadcast로 처리하므로 여기서는 제외
    snprintf(client->game_state.opponent_disconnect_message, sizeof(client->game_state.opponent_disconnect_message),
             "Game ended by %s.\n%s team wins the game!", end_reason, winner_text);

    client->game_state.opponent_disconnected = true;  // 게임 종료 상태를 나타내는 플래그로 재사용

    // 채팅 알림 추가 (기권 제외)
    snprintf(notification, sizeof(notification),
             "Game ended by %s. %s team wins!", end_reason, winner_text);
    add_chat_message_safe("System", notification);

    // 화면 업데이트 요청
    client->screen_update_requested = true;
    pthread_mutex_unlock(&screen_mutex);

    // 게임 상태 초기화
    // 필요한 경우 게임 상태 정리
    LOG_DEBUG("Game state cleanup after game end");

    LOG_INFO("Returned to main screen after game end");
    return 0;
}
